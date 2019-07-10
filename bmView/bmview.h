#pragma once
#include "intfs.h"
#include <QOpenGLWidget>
#include "ui_bmview.h"
#include "tilemap.h"

QT_FORWARD_DECLARE_CLASS(QOpenGLShaderProgram)

class bmView : public QOpenGLWidget, 
	protected QOpenGLFunctions, 
	public IGlobalRenderer,
	public std::enable_shared_from_this<bmView>
{
	Q_OBJECT
signals:
	void updateRenderer();
public:
	bmView(QWidget *parent = Q_NULLPTR);
protected:
	void initializeGL() override;
	void paintGL() override;
	void resizeGL(int width, int height) override;

	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;
private slots:
	void onRendererUpdate();
protected: //IGlobalRenderer
	void init() override;
	uint getZoomLevel() override;
	QPointF getCenter() override;
	uint getWidth() override;
	uint getHeight() override;
	void repaint() override;
	QVector3D screenToWorld(const int& nX, const int& nY) override;
private:
	Ui::bmViewClass ui;
private:
	QMatrix4x4 m_proj;
	QMatrix4x4 m_camera;
	QVector3D m_qvCameraPos;
	QPoint m_qpLastPos;
	ITileMapPtr m_pTiles = nullptr;
	uint m_uiZoomLevel = 12;
private:
	double getZoomFactor();
	void updateZoomLevel(const int& delta);
};
