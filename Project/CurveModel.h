#pragma once
#include <osg/Vec3>
#include <osg/Geometry>
#include <osgViewer/Viewer>
#include <QMainWindow>
#include <vector>

// 自定义射线结构体
struct Ray {
    osg::Vec3 origin;
    osg::Vec3 direction;
    Ray(const osg::Vec3& o, const osg::Vec3& d) : origin(o), direction(d) {}
};

class CurvedRoadGenerator : public QMainWindow {
    Q_OBJECT
public:
    CurvedRoadGenerator(QWidget* parent = nullptr);
    void generateCurvedRoad(const osg::Vec3& A, const osg::Vec3& B, const osg::Vec3& C, 
                          float width, const osg::Vec3& normal);

private:
    // OSG可视化组件
    osg::ref_ptr<osgViewer::Viewer> viewer;
    osg::ref_ptr<osg::Group> root;
    
    // 算法核心函数
    osg::Vec3 computePerpendicularVector(const osg::Vec3& vec, const osg::Vec3& normal);
    Ray createRay(const osg::Vec3& p1, const osg::Vec3& p2);
    osg::Vec3 findClosestPoint(const Ray& ray, const osg::Vec3& point);
    osg::Vec3 findMiddlePoint(const osg::Vec3& p1, const osg::Vec3& p2);
    osg::Vec3 mirrorVector(const osg::Vec3& v1, const osg::Vec3& v2);
    
    // 可视化辅助函数
    void drawPoint(const osg::Vec3& pos, const osg::Vec4& color);
    void drawLine(const osg::Vec3& start, const osg::Vec3& end, const osg::Vec4& color);
};