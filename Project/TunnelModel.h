#pragma once
#include <osg/Geode>
#include <osg/Geometry>
#include <osgViewer/Viewer>
#include <QMainWindow>
#include <vector>
#include <cmath>

// 隧道参数结构体
struct TunnelParameters {
    float entranceLength;    // 洞口长度
    float entranceWidth;     // 洞口宽度
    float tunnelRadius;      // 隧道半径
    float precision;         // 插值精度
    float heightThreshold;   // 高程阈值B
    float extensionLength;   // 延伸长度C
    int textureType;         // 纹理类型
};

// 地形数据结构
struct TerrainData {
    osg::ref_ptr<osg::HeightField> heightField;
    osg::Vec3Array* vertices;
};

class TunnelBuilder : public QMainWindow {
    Q_OBJECT
public:
    TunnelBuilder(QWidget* parent = nullptr);
    void initializeScene();
    void computeHighGroundAreas();
    void buildTunnelGeometry();
    void modifyTerrain();

private:
    // OSG场景组件
    osg::ref_ptr<osgViewer::Viewer> viewer;
    osg::ref_ptr<osg::Group> root;
    osg::ref_ptr<osg::Geode> terrainGeode;
    
    // 算法中间数据
    TunnelParameters params;
    TerrainData terrain;
    std::vector<osg::Vec3> highGroundPoints;
    osg::Vec3 tunnelEntrance;
    osg::Vec3 tunnelExit;

    // 辅助函数
    bool isHighGround(float x, float y);
    osg::Vec3 computeTunnelEntrance();
    osg::Vec3 computeTunnelExit();
    void generateTunnelMesh(const osg::Vec3& start, const osg::Vec3& end);
    void applyTexture(osg::Geometry* geom);
    void carveTerrain(const osg::Vec3& pos, float radius);
};