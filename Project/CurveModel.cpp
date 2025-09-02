#include "CurvedRoadGenerator.h"
#include <osg/Geode>
#include <osg/ShapeDrawable>
#include <QHBoxLayout>
#include <cmath>

// 构造函数初始化
CurvedRoadGenerator::CurvedRoadGenerator(QWidget* parent) : QMainWindow(parent) {
    // Qt窗口设置
    setGeometry(100, 100, 1200, 800);
    QWidget* centralWidget = new QWidget(this);
    QHBoxLayout* layout = new QHBoxLayout(centralWidget);
    
    // OSG场景初始化
    viewer = new osgViewer::Viewer();
    root = new osg::Group();
    
    // 示例参数
    osg::Vec3 A(0, 0, 0);
    osg::Vec3 B(10, 5, 0);
    osg::Vec3 C(20, 0, 0);
    float width = 2.0f;
    osg::Vec3 normal(0, 0, 1);
    
    // 生成弯道路面
    generateCurvedRoad(A, B, C, width, normal);
    
    // 设置场景数据
    viewer->setSceneData(root);
    viewer->realize();
}

// 主算法实现
void CurvedRoadGenerator::generateCurvedRoad(const osg::Vec3& A, const osg::Vec3& B, 
                                           const osg::Vec3& C, float width, 
                                           const osg::Vec3& normal) {
    // 步骤1-2: 计算路径向量
    osg::Vec3 Vab = B - A;
    osg::Vec3 Vbc = C - B;

    // 步骤3-8: 计算扩展点
    osg::Vec3 V0 = computePerpendicularVector(Vab, normal) * width;
    osg::Vec3 V1 = computePerpendicularVector(Vbc, normal) * width;
    
    osg::Vec3 Ax = A + V0;
    osg::Vec3 ABx = B + V0;
    osg::Vec3 BCx = B + V1;
    osg::Vec3 Cx = C + V1;

    // 可视化扩展点
    drawPoint(Ax, osg::Vec4(1,0,0,1));  // 红色
    drawPoint(ABx, osg::Vec4(0,1,0,1)); // 绿色
    drawPoint(BCx, osg::Vec4(0,0,1,1)); // 蓝色
    drawPoint(Cx, osg::Vec4(1,1,0,1));  // 黄色

    // 步骤9-11: 计算中垂线
    osg::Vec3 AABx = ABx - Ax;
    osg::Vec3 BCxC = Cx - BCx;
    osg::Vec3 ABCM = mirrorVector(AABx, BCxC);

    // 步骤13-14: 创建射线
    Ray R1 = createRay(Ax, ABx);
    Ray R2 = createRay(BCx, Cx);

    // 步骤15-20: 计算关键点
    osg::Vec3 P1 = findClosestPoint(R1, R2.origin);
    osg::Vec3 P2 = findClosestPoint(R2, R1.origin);
    osg::Vec3 M12 = findMiddlePoint(P1, P2);
    osg::Vec3 P3 = M12 + (ABCM * width);
    
    osg::Vec3 PS = findClosestPoint(R1, P3);
    osg::Vec3 PE = findClosestPoint(R2, P3);

    // 可视化关键点
    drawPoint(PS, osg::Vec4(1,0,1,1)); // 紫色
    drawPoint(PE, osg::Vec4(0,1,1,1)); // 青色

    // 生成路面几何体
    osg::ref_ptr<osg::Geode> roadGeode = new osg::Geode();
    osg::ref_ptr<osg::Geometry> roadGeom = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    
    // 添加路面顶点（示例四边形）
    vertices->push_back(Ax);
    vertices->push_back(ABx);
    vertices->push_back(BCx);
    vertices->push_back(Cx);
    
    roadGeom->setVertexArray(vertices);
    roadGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, 4));
    roadGeode->addDrawable(roadGeom);
    root->addChild(roadGeode);
}

// 计算垂直向量（归一化）
osg::Vec3 CurvedRoadGenerator::computePerpendicularVector(const osg::Vec3& vec, 
                                                        const osg::Vec3& normal) {
    osg::Vec3 cross = vec ^ normal; // OSG的叉乘运算符
    return cross.normalize();
}

// 创建射线
Ray CurvedRoadGenerator::createRay(const osg::Vec3& p1, const osg::Vec3& p2) {
    return Ray(p1, p2 - p1);
}

// 计算射线上最近点
osg::Vec3 CurvedRoadGenerator::findClosestPoint(const Ray& ray, 
                                              const osg::Vec3& point) {
    osg::Vec3 diff = point - ray.origin;
    float t = diff * ray.direction; // 点积计算投影参数
    return ray.origin + ray.direction * t;
}

// 计算中点
osg::Vec3 CurvedRoadGenerator::findMiddlePoint(const osg::Vec3& p1, 
                                             const osg::Vec3& p2) {
    return (p1 + p2) * 0.5f;
}

// 镜像向量计算（中垂线方向）
osg::Vec3 CurvedRoadGenerator::mirrorVector(const osg::Vec3& v1, 
                                          const osg::Vec3& v2) {
    // 计算两个向量的平均方向
    osg::Vec3 sum = v1 + v2;
    return sum.normalize();
}

// 可视化辅助函数：绘制点
void CurvedRoadGenerator::drawPoint(const osg::Vec3& pos, 
                                   const osg::Vec4& color) {
    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    osg::ref_ptr<osg::ShapeDrawable> shape = new osg::ShapeDrawable(
        new osg::Sphere(pos, 0.1f));
    shape->setColor(color);
    geode->addDrawable(shape);
    root->addChild(geode);
}

// 可视化辅助函数：绘制线
void CurvedRoadGenerator::drawLine(const osg::Vec3& start, 
                                  const osg::Vec3& end, 
                                  const osg::Vec4& color) {
    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    
    vertices->push_back(start);
    vertices->push_back(end);
    geom->setVertexArray(vertices);
    geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, 2));
    
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    colors->push_back(color);
    geom->setColorArray(colors, osg::Array::BIND_OVERALL);
    
    geode->addDrawable(geom);
    root->addChild(geode);
}

// Qt主函数
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    CurvedRoadGenerator window;
    window.show();
    return app.exec();
}