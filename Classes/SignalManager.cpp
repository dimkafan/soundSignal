//
//  SignalManager.cpp
//  trapSound
//
//  Created by Дима on 25.02.15.
//
//

#include "SignalManager.h"

#include <set>
#include <CoreAudio/CoreAudioTypes.h>
#include "SignalAnalitic.h"

const int AUDIO_BUFFERS = 16;
static std::set<void*> s_signals;

void audioDelegate(
                   void *                          inUserData,
                   AudioQueueRef                   inAQ,
                   AudioQueueBufferRef             inBuffer,
                   const AudioTimeStamp *          inStartTime,
                   UInt32                          inNumberPacketDescriptions,
                   const AudioStreamPacketDescription *inPacketDescs)
{
    auto it = s_signals.find(inUserData);
    if (it != s_signals.end())
    {
        SignalManager* pMan = reinterpret_cast<SignalManager*>(*it);
        pMan->audioCallback(inUserData, inAQ, inBuffer, inStartTime, inNumberPacketDescriptions, inPacketDescs);
    }
}

SignalManager::SignalManager(size_t signalSize, float timePoint, float xTime):
_queue(nullptr)
,_bufferCount(AUDIO_BUFFERS)
,_onRecieve(nullptr)
,_avrData(signalSize,0)
,_pointTime(timePoint)
,_rawData(AUDIO_BUFFERS)
,_sndBuffLen(xTime)
{
    
}

SignalManager::~SignalManager()
{
    if (_queue)
        AudioQueueDispose(_queue, true);
}

void SignalManager::init()
{
    AudioStreamBasicDescription mDataFormat;
    mDataFormat.mFormatID = kAudioFormatLinearPCM;
    mDataFormat.mSampleRate = 44100;
    mDataFormat.mChannelsPerFrame = 1;
    mDataFormat.mBitsPerChannel = 16;
    mDataFormat.mBytesPerPacket =
    mDataFormat.mBytesPerFrame = mDataFormat.mChannelsPerFrame * sizeof(SignalDataType);
    mDataFormat.mFramesPerPacket = 1;
    mDataFormat.mFormatFlags = kLinearPCMFormatFlagIsBigEndian | kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
    _dt = 1.f/mDataFormat.mSampleRate;
    
    AudioQueueRef queue = nullptr;
    AudioQueueNewInput(&mDataFormat, audioDelegate, this, NULL, kCFRunLoopCommonModes, 0, &queue);
    
    if (queue)
    {
        unsigned long frameSize = ceil(_sndBuffLen*mDataFormat.mSampleRate);
        std::vector<AudioQueueBufferRef> aBuffers(_bufferCount, nullptr);

        for (size_t i=0; i< aBuffers.size(); i++)
        {
            AudioQueueAllocateBuffer(queue, frameSize* mDataFormat.mBytesPerFrame, &aBuffers[i]);
            aBuffers[i]->mUserData = reinterpret_cast<void*>(i);
            AudioQueueEnqueueBuffer(queue, aBuffers[i], 0, NULL);
            _rawData[i].resize(frameSize);
        }
        _queue = queue;
        s_signals.insert(this);
    }
}

void SignalManager::start()
{
    if (_queue)
        AudioQueueStart(_queue, NULL);
}

void SignalManager::pause()
{
    if (_queue)
        AudioQueuePause(_queue);//AudioQueueStop, true
}

void SignalManager::setSoundBuffTime(float timeSec)
{
    AudioQueueStop(_queue, true);
    AudioQueueDispose(_queue, true);
    _queue = nullptr;
    s_signals.erase(this);
    _sndBuffLen = timeSec;
    init();
    start();
}

void SignalManager::setTimeInPoint(float pointTime)
{
    AudioQueueStop(_queue, true);
    AudioQueueDispose(_queue, true);
    _queue = nullptr;
    std::fill(_avrData.begin(), _avrData.end(), 0);
    _nextIdx = 0;
    s_signals.erase(this);
    
    _pointTime = pointTime;
    float prop = _pointTime/_dt;
    if (prop <= 1.5f)
        _pointTime = _dt;
    init();
    start();
}

float SignalManager::getTimeInPoint() const
{
    return _pointTime;
}

void SignalManager::setXTime(float xTime)
{
    setTimeInPoint(xTime/_avrData.size());
}

void SignalManager::audioCallback(
                   void *                          inUserData,
                   AudioQueueRef                   inAQ,
                   AudioQueueBufferRef             inBuffer,
                   const AudioTimeStamp *          inStartTime,
                   UInt32                          inNumberPacketDescriptions,
                   const AudioStreamPacketDescription *inPacketDescs)
{
    if (this != inUserData)
        return;
    if (inNumberPacketDescriptions == 0)
        return;
    _recieveCount++;
    long buffIdx = reinterpret_cast<long>(inBuffer->mUserData);
    auto& rawBuff = _rawData[buffIdx];
    memcpy(&rawBuff[0], inBuffer->mAudioData, inBuffer->mAudioDataByteSize);
    AudioQueueEnqueueBuffer(_queue, inBuffer, 0, NULL);
    
    
    _nextIdx = SignalAnalitic::avarage(rawBuff, inNumberPacketDescriptions, _dt, _pointTime, _avrData, _nextIdx);
    SignalAnalitic::getMinMax(_avrData, _minY, _maxY);
    SignalAnalitic::FFT(_avrData, _nextIdx, _pointTime, _spectr);
    
    std::fill(rawBuff.begin(), rawBuff.begin() + inNumberPacketDescriptions , 0);
    
    if (_onRecieve)
        _onRecieve(_recieveCount);
}

void SignalManager::setOnRecieveFunction(const std::function<void(long long)>& func)
{
    _onRecieve = func;
}

const std::vector<SignalDataType>& SignalManager::getAvrSignal(size_t& startIdx) const
{
    startIdx = _nextIdx;
    return _avrData;
}

const Spectr& SignalManager::getSpectr() const
{
    return _spectr;
}

SignalDataType SignalManager::getMinY() const
{
    return _minY;
}

SignalDataType SignalManager::getMaxY() const
{
    return _maxY;
}

float SignalManager::getXTime() const
{
    return _pointTime*_avrData.size();
}

void SignalManager::addAvrData(const std::vector<SignalDataType>& source, int buffIdx)
{
    size_t delta = _avrData.size() - _nextIdx;
    size_t sourceIdx = 0;
    if (source.size() > _avrData.size())
        sourceIdx = source.size() - _avrData.size();
    size_t deltaSource = source.size() - sourceIdx;
    
//    while (deltaSource > 0)
//    {
//        if (delta > deltaSource)
//        {
//            memcpy(&_avrData[_nextIdx], &source[sourceIdx], deltaSource*sizeof(SignalDataType));
//            _nextIdx += deltaSource;
//            sourceIdx += deltaSource;
//            deltaSource = source.size() - sourceIdx;
//        }
//    }

    size_t destSize = _avrData.size();
    for(auto& item : source)
    {
        _avrData[_nextIdx] = item;
        _nextIdx++;
        if (_nextIdx >= destSize)
            _nextIdx = 0;
    }
    
    SignalAnalitic::getMinMax(_avrData, _minY, _maxY);
}
