#pragma once
#include <osg/Geode>
#include <osg/Geometry>
#include <osgViewer/Viewer>
#include <QMainWindow>
#include <vector>

// 边坡参数结构体
struct SlopeParameters {
    float baseElevation;    // 地基基准高程
    float slopeAngle;       // 边坡角度（度）
    int stages;             // 边坡级数
    float platformWidth;    // 平台宽度
    float ditchWidth;       // 排水沟宽度
    float gridSize;         // 格网分割尺寸
};

// 交点数据结构
struct IntersectionPoint {
    osg::Vec3 point;
    bool isBoundary;
};

// 微分单元结构体
struct MicroUnit {
    osg::Vec3 vertices[4];  // 四个顶点坐标
    int property;           // 属性标识
};

// 三维体块结构体
struct SlopeBlock {
    osg::ref_ptr<osg::Geometry> geometry;
    int property;
};

class SlopeModeler : public QMainWindow {
    Q_OBJECT
public:
    SlopeModeler(QWidget* parent = nullptr);
    void initializeScene();
    void computeSlopeRange();
    void gridSegmentation();
    void unitClassification();
    void build3DBlocks();
    void validateAndMerge();

private:
    // OSG场景组件
    osg::ref_ptr<osgViewer::Viewer> viewer;
    osg::ref_ptr<osg::Group> root;
    osg::ref_ptr<osg::Geode> terrainGeode;
    
    // 算法中间数据
    SlopeParameters params;
    std::vector<IntersectionPoint> intersections;
    std::vector<std::vector<MicroUnit>> gridUnits;
    std::vector<SlopeBlock> slopeBlocks;

    // 辅助函数
    osg::Geometry* createCrossSection(float offset);
    void computeIntersections(osg::Geometry* crossSection);
    void createTopologySurface();
    MicroUnit createMicroUnit(const osg::Vec3& v1, const osg::Vec3& v2, 
                            const osg::Vec3& v3, const osg::Vec3& v4);
    void mergeAdjacentUnits();
    void applyTexture(osg::Geometry* geom, int property);
};