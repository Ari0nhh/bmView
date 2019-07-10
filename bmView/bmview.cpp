#include "bmview.h"
#include "consts.h"

bmView::bmView(QWidget *parent)
	: QOpenGLWidget(parent)
{
	ui.setupUi(this);

	connect(this, &bmView::updateRenderer, this, &bmView::onRendererUpdate);
}

void bmView::init()
{
	auto pRender = std::dynamic_pointer_cast<IGlobalRenderer>(shared_from_this());
	m_pTiles = std::make_shared<CTileMap>(pRender);
	
	this->showMaximized();
}

void bmView::initializeGL()
{
	initializeOpenGLFunctions();

	glClearColor(0x00, 0x00, 0x00, 0xFF);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_qvCameraPos = { 55948.87f, 54734.17f, -1000.f };
	m_camera.setToIdentity();
	m_camera.translate(m_qvCameraPos);
	
	m_pTiles->init();
	m_pTiles->initGL();

	m_pTiles->move();
	m_pTiles->detail(m_uiZoomLevel);
}

void bmView::paintGL()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if (!m_pTiles)
		return;

	double dbScale = -1.f * m_qvCameraPos.z() / (gfMaxPerspective - gfMinPerspective);
	auto qmWorld = m_proj * m_camera;

	m_pTiles->draw(qmWorld);
}

void bmView::resizeGL(int width, int height)
{
	m_proj.setToIdentity();
	m_proj.perspective(45.0f, GLfloat(width) / height, gfMinPerspective, gfMaxPerspective);
	m_pTiles->rebuild();
}

void bmView::mousePressEvent(QMouseEvent* event)
{
	m_qpLastPos = event->pos();
}

void bmView::mouseMoveEvent(QMouseEvent* event)
{
	if ((m_qpLastPos.x() == event->x()) && (m_qpLastPos.y() == event->y()))
		return;

	if (event->buttons() & Qt::LeftButton) {
		auto qvPrev = screenToWorld(m_qpLastPos.x(), m_qpLastPos.y());
		auto qvNow = screenToWorld(event->x(), event->y());

		auto dx = (qvNow.x() - qvPrev.x());
		auto dy = (qvNow.y() - qvPrev.y());

		m_qvCameraPos.setX(m_qvCameraPos.x() + dx);
		m_qvCameraPos.setY(m_qvCameraPos.y() + dy);

		m_camera.setToIdentity();
		m_camera.translate(m_qvCameraPos);

		m_pTiles->move();

		update();
	}

	m_qpLastPos = event->pos();
}

void bmView::wheelEvent(QWheelEvent* event)
{
	int delta = event->delta();
	auto fNewZ = m_qvCameraPos.z() + delta * getZoomFactor();

	if (fNewZ > -1 * gfMinPerspective || fNewZ < -1 * gfMaxPerspective)
		return;

	m_qvCameraPos.setZ(fNewZ);
	m_camera.setToIdentity();
	m_camera.translate(m_qvCameraPos);

	updateZoomLevel(delta);

	m_pTiles->rebuild();
	m_pTiles->detail(m_uiZoomLevel);
	update();
}

void bmView::onRendererUpdate()
{
	this->update();
}

glm::uint bmView::getZoomLevel()
{
	return m_uiZoomLevel;
}

QPointF bmView::getCenter()
{
	return { m_qvCameraPos.x() / 1000.f, m_qvCameraPos.y() / 1000.f };
}

glm::uint bmView::getWidth()
{
	return this->width();
}

glm::uint bmView::getHeight()
{
	return this->height();
}

void bmView::repaint()
{
	emit updateRenderer();
}

QVector3D bmView::screenToWorld(const int& nX, const int& nY)
{
	glm::dvec4 glmViewPort(0, 0, width(), height());
	glm::dmat4x4 glmCam(1.0);
	glmCam = glm::translate(glmCam, glm::dvec3(m_qvCameraPos.x(), m_qvCameraPos.y(), m_qvCameraPos.z()));

	glm::dmat4x4 glmProj = glm::perspective((double)glm::radians(45.f), (double)width() / height(),
		(double)gfMinPerspective, (double)gfMaxPerspective);

	glm::dvec3 nearP(nX, height() - nY, 0.f);
	nearP = glm::unProject(nearP, glmCam, glmProj, glmViewPort);

	glm::dvec3 farP(nX, height() - nY, 1.f);
	farP = glm::unProject(farP, glmCam, glmProj, glmViewPort);

	double worldZ = -1.f * (double)m_qvCameraPos.z();
	double t = (worldZ - gfMinPerspective) / ((double)gfMaxPerspective - gfMinPerspective);

	auto glmResult = farP * t + nearP * (1.f - t);


	return QVector3D(glmResult.x, glmResult.y, m_qvCameraPos.z());
}

double bmView::getZoomFactor()
{
	double dbZoom = -100 * m_qvCameraPos.z() / ((double)gfMaxPerspective - gfMinPerspective);
	double dbResult = 199.9 / 1.9 - 99.0 / (1.9 * dbZoom);

	if (dbResult < 1.0)
		dbResult = 1.0;

	if (dbResult > 100.0)
		dbResult = 100.0;

	return dbResult;
}

void bmView::updateZoomLevel(const int& delta)
{
	if (delta < 0) {
		m_uiZoomLevel = std::max(2u, m_uiZoomLevel - 1u);
	}
	else {
		m_uiZoomLevel = std::min(18u, m_uiZoomLevel + 1u);
	}
}
