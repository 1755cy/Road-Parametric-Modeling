#pragma once
#include <osg/Geode>
#include <osg/Geometry>
#include <osgViewer/Viewer>
#include <QMainWindow>
#include <vector>
#include <cmath>

// 桥梁参数结构体
struct BridgeParameters {
    // 桥头参数
    float headLength;           // 桥头延长长度
    float headElevation;        // 桥头标高
    
    // 桥面参数
    float deckWidth;            // 桥面宽度
    float deckThickness;        // 桥面厚度
    int deckTextureType;        // 桥面纹理类型
    
    // 桥墩参数
    float pierSpacing;          // 桥墩间距
    float pierBaseWidth;        // 桥墩底座宽度
    float pierHeight;           // 桥墩高度
    int pierTextureType;        // 桥墩纹理类型
};

// 地形数据结构
struct TerrainData {
    osg::ref_ptr<osg::HeightField> heightField;
    osg::Vec3Array* vertices;
};

// 桥梁部件枚举
enum BridgeComponent {
    DECK = 0,
    PIER,
    RAILING,
    ABUTMENT
};

class BridgeBuilder : public QMainWindow {
    Q_OBJECT
public:
    BridgeBuilder(QWidget* parent = nullptr);
    void initializeScene();
    void computeLowLyingAreas();
    void buildBridgeGeometry();
    void applyTextures();

private:
    // OSG场景组件
    osg::ref_ptr<osgViewer::Viewer> viewer;
    osg::ref_ptr<osg::Group> root;
    osg::ref_ptr<osg::Geode> terrainGeode;
    
    // 算法中间数据
    BridgeParameters params;
    TerrainData terrain;
    std::vector<osg::Vec3> lowLyingPoints;
    std::vector<osg::Vec3> bridgeDeckPoints;
    std::vector<osg::Vec3> pierPositions;

    // 辅助函数
    float getPlanHeight(float x, float y);
    bool isLowLyingArea(float x, float y);
    osg::Vec3 computeBridgeHeadPosition(const osg::Vec3& start, float length);
    void createPierGeometry(const osg::Vec3& position);
    void createDeckGeometry();
    osg::Texture2D* loadTexture(int type);
};