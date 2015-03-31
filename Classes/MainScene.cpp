#include <CoreAudio/CoreAudioTypes.h>
#include <math.h>
//#include "GameHelper/AndroidFix.h"
#include "MainScene.h"
#include "SignalTypes.h"

const int AUDIO_BUFFERS = 16;
const size_t BUFFER_SIZE = 2048;


std::string floatToStr(float val)
{
    std::string res = std::to_string(val);
    size_t pos = res.length();
    auto it = res.rbegin();
    for(;it != res.rend(); ++it)
        if(*it != '0')
            break;
        else
            pos--;
    if (pos != 0 && res.at(pos-1) == '.')
        pos--;
    res.erase(res.begin() + pos, res.end());
    return std::move(res);
}

USING_NS_CC;

MainScene::MainScene(): _rawSignal(AUDIO_BUFFERS, BUFFER_SIZE, 0.0001f)
,_count(nullptr)
,_pause(nullptr)
{

}

Scene* MainScene::createScene()
{
    // 'scene' is an autorelease object
    auto scene = Scene::create();
    
    // 'layer' is an autorelease object
    auto layer = MainScene::create();

    // add layer as a child to scene
    scene->addChild(layer);

    // return the scene
    return scene;
}

// on "init" you need to initialize your instance
bool MainScene::init()
{
    //////////////////////////////
    // 1. super init first
    if ( !Node::init() )
    {
        return false;
    }
    
    cocos2d::spritebuilder::CCBXReader *reader = cocos2d::spritebuilder::CCBXReader::createFromFile("Interface/MainScene.ccbi");
    cocos2d::Node * node = reader->createNode(this, cocos2d::spritebuilder::SceneScaleType::MAXSCALE);
    addChild(node);
    
    auto bbox = _node_graphic->getBoundingBox();
    cocos2d::spritebuilder::CCBXReader *pointreader = cocos2d::spritebuilder::CCBXReader::createFromFile("Interface/dataPoint.ccbi");
    _points.reserve(BUFFER_SIZE);
    for(size_t i = 0; i < BUFFER_SIZE; i++)
    {
        cocos2d::Node * node = pointreader->createNode(this);
        _node_graphic->addChild(node);
        _points.push_back(node);
    }
    
    initAudio();
    drawAxis(_node_graphic);
    
    return true;
}

MainScene::~MainScene()
{

}

void MainScene::initAudio()
{
    _rawSignal.init();
    _rawSignal.setOnRecieveFunction([this](long long count)
                                    {this->onRecieveSignal(count);});
                                        
    _rawSignal.start();
}

cocos2d::spritebuilder::ccReaderClickCallback MainScene::onResolveCCBClickSelector(const std::string &selectorName, cocos2d::Node* node)
{
    CCBX_SELECTORRESOLVER_CLICK_GLUE(this, "pause", MainScene::onPause);
    CCBX_SELECTORRESOLVER_CLICK_GLUE(this, "resume", MainScene::onResume);
    return nullptr;
}

bool MainScene::onAssignCCBMemberVariable(const std::string &memberVariableName, cocos2d::Node* node)
{
    CCBX_MEMBERVARIABLEASSIGNER_GLUE("count", _count);
    CCBX_MEMBERVARIABLEASSIGNER_GLUE("pause", _pause);
    CCBX_MEMBERVARIABLEASSIGNER_GLUE("node_signal", _node_graphic);
    CCBX_MEMBERVARIABLEASSIGNER_GLUE("node_fft", _node_fft);
    
    CCBX_MEMBERVARIABLEASSIGNER_GLUE("maxY", _maxY);
    CCBX_MEMBERVARIABLEASSIGNER_GLUE("minY", _minY);
    CCBX_MEMBERVARIABLEASSIGNER_GLUE("x_time", _xTime);
    
    return true;
}

void MainScene::onRecieveSignal(long long count)
{
    _count->setString(std::to_string(count));
    size_t startIdx = 0;
    auto data = _rawSignal.getAvrSignal(startIdx);
    auto minY = _rawSignal.getMinY();
    auto maxY = _rawSignal.getMaxY();
    
    auto sz = _node_graphic->getContentSize();
    _maxY->setString(std::to_string(maxY));
    _minY->setString(std::to_string(minY));
    _xTime->setString(floatToStr(_rawSignal.getXTime()));
    
    float fx = sz.width/data.size();
    
    float fy = (maxY - minY) > 0 ? sz.height/2/(maxY - minY) : 0;
    size_t maxIdx = std::max(data.size(), _points.size());
    for(size_t i = 0; i < maxIdx; i++)
    {
        auto& dataItem = data.at(i);
        auto pItem = _points.at(i);
        size_t xIdx = i < startIdx ? i + maxIdx : i;
        pItem->setPosition(fx*(xIdx - startIdx), sz.height/2 + fy*dataItem);
        pItem->setVisible(true);
    }
    for(auto it = _points.begin() + maxIdx; it != _points.end(); ++it)
    {
        (*it)->setVisible(false);
    }
}

void MainScene::onPause(cocos2d::Ref* target)
{
    _rawSignal.pause();
    _pause->setVisible(false);
}

void MainScene::onResume(cocos2d::Ref* target)
{
    _rawSignal.start();
    _pause->setVisible(true);
}

void MainScene::drawAxis(cocos2d::Node* node)
{
    DrawNode* axisNode =DrawNode::create();
    auto bbox = node->getBoundingBox();
    float deltaBreak = 5;
    //draw x
    float  x = 0;
    while(x < bbox.size.width)
    {
        axisNode->drawLine(Vec2(x, bbox.size.height/2),Vec2(x + deltaBreak, bbox.size.height/2),Color4F(1,0,0,1));
        x += 2*deltaBreak;
    }
    
    float xTime = _rawSignal.getXTime();
    float scX = bbox.size.width/xTime;
    float pow10 = ::log10f(xTime);
    if (pow10 > 0)
        pow10 = static_cast<int>(pow10);
    else if (pow10 < 0)
        pow10 = static_cast<int>(pow10 - 1.f);
    pow10 = ::powf(10.f, pow10);
    int maxX = ceil(xTime/pow10);
    scX *= pow10;
    for(int i = 0; i < maxX; i++)
    {
        float y = 0;
        while (y < bbox.size.height)
        {
            axisNode->drawLine(Vec2(i*scX, y), Vec2(i*scX, y + deltaBreak), Color4F(1,0,0,1));
            y += 2*deltaBreak;
        }
    }
    _node_graphic->addChild(axisNode);
}
