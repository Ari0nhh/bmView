#include "tilemap.h"
#include "consts.h"
#include "geotex.h"

CTile::CTile(ITileMapPtr pMap) : m_pMap(pMap)
{
	
}

void CTile::initGL()
{
	InitGLBuffers();
	InitShaders();
}

std::pair<uint, uint> CTile::getIndex()
{
	return m_spIndex;
}

void CTile::setIndex(const std::pair<uint, uint>& spIndex)
{
	m_spIndex = spIndex;
}

QPoint CTile::getTileIndex()
{
	return m_qpTileIndex;
}

void CTile::setTileIndex(const QPoint& qpIndex, const uint& uiZoom)
{
	m_qpTileIndex = qpIndex;
	m_uiZoomLevel = uiZoom;
		
	invalidateTexture();
}

QVector3D CTile::getPos()
{
	return m_qvPos;
}

QVector3D CTile::getSize()
{
	return m_qvSize;
}

void CTile::place(const QVector3D& qvPos, const QVector3D& qvSize)
{
	m_qvPos = qvPos;
	m_qvSize = qvSize;
}

void CTile::draw(const QMatrix4x4& qmWorld)
{
	bool bHaveTexture = false;
	
	if (m_pTexture && m_pTexture->valid()) {
		m_pTexture->bind();
		bHaveTexture = true;
	}
	else if (m_pPrevTexture && m_pPrevTexture->valid()) {
		m_pPrevTexture->bind();
		bHaveTexture = true;
	}

	if (!bHaveTexture)
		return;
	
	m_pVAO->bind();
	m_pShaders->bind();

	m_pShaders->setUniformValue(m_nWorldMatrixLoc, qmWorld);
	m_pShaders->setUniformValue(m_nPosLoc, m_qvPos);
	m_pShaders->setUniformValue(m_nSizeLoc, m_qvSize);

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void CTile::invalidate()
{
	m_bInvalidate = true;
	//if (m_pTexture && m_pTexture->valid())
		//m_pPrevTexture = m_pTexture;
	m_pTexture = nullptr;
}

bool CTile::InitGLBuffers()
{
	//0) Create and bind VAO
	m_pVAO = std::make_shared<QOpenGLVertexArrayObject>();
	if (!m_pVAO->create())
		return false;
	m_pVAO->bind();

	//1) Create & allocate index buffer
	{
		auto pDrawIndex = InitIndexBuffer();
		m_pEBO = std::make_shared<QOpenGLBuffer>(QOpenGLBuffer::IndexBuffer);
		if (!m_pEBO->create())
			return false;

		m_pEBO->bind();
		m_pEBO->setUsagePattern(QOpenGLBuffer::StaticDraw);
		m_pEBO->allocate(pDrawIndex.get(), 6 * sizeof(GLuint));
	}

	//2) Create and allocate vertex buffer
	{
		auto pVertexData = InitVertexBuffer();
		m_pVBO = std::make_shared<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);
		if (!m_pVBO->create())
			return false;

		m_pVBO->bind();
		m_pVBO->setUsagePattern(QOpenGLBuffer::StaticDraw);
		m_pVBO->allocate(pVertexData.get(), 4 * 4 * sizeof(GLfloat));
	}

	//4) Setup vertex coord and well positions buffer indexes
	auto* pFunc = QOpenGLContext::currentContext()->functions();

	//Coordinates
	pFunc->glEnableVertexAttribArray(0);
	//Texture coordinates
	pFunc->glEnableVertexAttribArray(1);

	m_pVBO->bind();
	pFunc->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr);
	pFunc->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), reinterpret_cast<void*>(2 * sizeof(GLfloat)));

	return true;
}

bool CTile::InitShaders()
{
	m_pShaders = std::make_shared<QOpenGLShaderProgram>();
	if (!m_pShaders->addShaderFromSourceCode(QOpenGLShader::Vertex, gwTileVS)) {
		auto sError = m_pShaders->log();
		return false;
	}

	if (!m_pShaders->addShaderFromSourceCode(QOpenGLShader::Fragment, gwTileFS)) {
		auto sError = m_pShaders->log();
		return false;
	}

	m_pShaders->bindAttributeLocation("position", 0);
	m_pShaders->bindAttributeLocation("texCoord", 1);

	if (!m_pShaders->link()) {
		return false;
	}

	m_pShaders->bind();

	m_nWorldMatrixLoc = m_pShaders->uniformLocation("world");
	m_nPosLoc = m_pShaders->uniformLocation("pos");
	m_nSizeLoc = m_pShaders->uniformLocation("size");

	m_pShaders->setUniformValue("map_data", 0);

	return true;
}

void CTile::onTextureReady()
{
	m_pPrevTexture = nullptr;
	auto pMap = m_pMap.lock();
	pMap->renderer()->repaint();
	m_bInvalidate = false;
}

std::shared_ptr<GLuint[]> CTile::InitIndexBuffer()
{
	auto pDrawIndex = new GLuint[6];
	//0 - 1 - 2 - 0 - 2 - 3
	pDrawIndex[0] = 0;
	pDrawIndex[1] = pDrawIndex[0] + 1;
	pDrawIndex[2] = pDrawIndex[0] + 2;
	pDrawIndex[3] = pDrawIndex[0];
	pDrawIndex[4] = pDrawIndex[0] + 2;
	pDrawIndex[5] = pDrawIndex[0] + 3;

	return std::shared_ptr<GLuint[]>(pDrawIndex);
}

std::shared_ptr<GLfloat[]> CTile::InitVertexBuffer()
{
	auto pVertexBuffer = new GLfloat[4 * 4];
	auto pPtr = pVertexBuffer;

	for (size_t i = 0; i < 4; ++i) {
		auto szVertIdx = 2 * i;

		pPtr[0] = gfRectMatrix[szVertIdx];
		pPtr[1] = gfRectMatrix[szVertIdx + 1];
		pPtr[2] = gfRectMatrix[szVertIdx];
		pPtr[3] = gfRectMatrix[szVertIdx + 1];

		pPtr += 4;
	}

	return std::shared_ptr<GLfloat[]>(pVertexBuffer);
}

void CTile::invalidateTexture()
{
	if (!m_bInvalidate)
		return;

	m_pTexture = nullptr;
	auto pProvider = CBingGeoTextureProvider::get();
	auto pMath = pProvider->getMath();

	auto qsQuad = pMath->tile2quad(m_qpTileIndex.rx(), m_qpTileIndex.ry(), m_uiZoomLevel);
	m_pTexture = pProvider->getTexture(qsQuad, std::bind(&CTile::onTextureReady, this));
	m_pTexture->init();
}

ITileCircularBuffer& CTileCircularBuffer::operator>>(const uint& uiCount)
{
	if (m_vTiles.size() < 2)
		return *this;

	for (uint i = 0; i < uiCount; ++i) {
		auto pItem = m_vTiles.back();
		
		pItem.lock()->invalidate();
		
		m_vTiles.erase(m_vTiles.end() - 1);
		m_vTiles.emplace(m_vTiles.begin(), pItem);
	}

	rebuild();
	return *this;
}

ITileCircularBuffer& CTileCircularBuffer::operator<<(const uint& uiCount)
{
	for (uint i = 0; i < uiCount; ++i) {
		auto pItem = m_vTiles.front();

		pItem.lock()->invalidate();

		m_vTiles.erase(m_vTiles.begin());
		m_vTiles.push_back(pItem);
	}

	rebuild();
	return *this;
}

void CTileCircularBuffer::addFirst(ITilePtr pTile)
{
	m_vTiles.emplace(m_vTiles.begin(), pTile);
}

void CTileCircularBuffer::addLast(ITilePtr pTile)
{
	m_vTiles.push_back(pTile);
}

std::vector<ITilePtr_>::const_iterator CTileCircularBuffer::begin()
{
	return m_vTiles.cbegin();
}

std::vector<ITilePtr_>::const_iterator CTileCircularBuffer::end()
{
	return m_vTiles.cend();
}

std::vector<ITilePtr_>::reverse_iterator CTileCircularBuffer::rbegin()
{
	return m_vTiles.rbegin();
}

std::vector<ITilePtr_>::reverse_iterator CTileCircularBuffer::rend()
{
	return m_vTiles.rend();
}

ITilePtr CTileCircularBuffer::at(const size_t& szIdx)
{
	if ((szIdx < 0) || szIdx > m_vTiles.size())
		return nullptr;

	return m_vTiles[szIdx].lock();
}

bool CTileCircularBuffer::empty()
{
	return m_vTiles.empty();
}

size_t CTileCircularBuffer::size()
{
	return m_vTiles.size();
}

void CTileCircularBuffer::rebuild()
{
	for (size_t i = 0; i < m_vTiles.size(); ++i) {
		auto pTile = m_vTiles[i].lock();
		if (pTile) {
			auto spIndex = pTile->getIndex();
			m_bVertical ? spIndex.second = i : spIndex.first = i;
			pTile->setIndex(spIndex);
		}
	}
}

CTileMap::CTileMap(IGlobalRendererPtr pRenderer) : m_pGlobal(pRenderer)
{
	auto pProv = CBingGeoTextureProvider::get();
	auto pMeta = pProv->getMetadata();
	if (!pMeta->valid())
		return;

	m_upZoomLevels = pMeta->getZoomLevels();
}

void CTileMap::init()
{
	auto pMap = std::dynamic_pointer_cast<ITileMap>(shared_from_this());
	//0) Сначала создадим кольцевые буферы для строк и колонок
	for (size_t i = 0; i < 6; ++i)
		m_vRows.push_back(std::make_shared<CTileCircularBuffer>(false));

	for (size_t i = 0; i < 10; ++i)
		m_vCols.push_back(std::make_shared<CTileCircularBuffer>(true));

	//1) Потом запилим сами тайлы и распихаем их по буферам
	for (size_t i = 0; i < m_vRows.size() * m_vCols.size(); ++i) {
		ITilePtr pTile = std::make_shared<CTile>(pMap);
		m_vTiles.push_back(pTile);

		size_t nRow = i / m_vCols.size();

		size_t nCol = i % m_vCols.size();
		bool bLast = ((m_vCols.size() * m_vRows.size() - 1) == i);

		m_vRows[nRow]->addLast(pTile);
		m_vCols[nCol]->addLast(pTile);
	}
	
	//2) Перестроим индексы
	for (auto it : m_vCols)
		it->rebuild();

	for (auto it : m_vRows)
		it->rebuild();

}

void CTileMap::initGL()
{
	for (auto pTile : m_vTiles)
		pTile->initGL();
}

void CTileMap::draw(const QMatrix4x4& qmWorld)
{
	for (auto it : m_vTiles)
		it->draw(qmWorld);
}

bool CTileMap::detail(const uint& uiZoomLevel)
{
	if ((uiZoomLevel < m_upZoomLevels.first) || (uiZoomLevel > m_upZoomLevels.second))
		return false;

	m_uiZoomLevel = uiZoomLevel;

	//0) Сбрасываем текстуры всех тайлов
	for (auto it : m_vTiles)
		it->invalidate();

	//1) Определяем индекс тайла, к которому относится координата камеры (центра карты)
	auto pRender = m_pGlobal.lock();
	auto qpCenter = pRender->getCenter();
	auto pMath = CBingGeoTextureProvider::get()->getMath();
	auto pPixCoord = pMath->wgs2pix(qpCenter.rx(), qpCenter.ry(), m_uiZoomLevel);
	auto ptIdx = pMath->pix2tile(pPixCoord.first, pPixCoord.second);

	//2) Определяем диапазон индексов для данного УД
	auto nTileCount = pMath->getTileIndexRange(m_uiZoomLevel);

	//3) Считаем индекс для нулевого тайла
	std::pair<int, int> spIdx0 = std::make_pair(ptIdx.first - ((int)m_vCols.size() / 2 - 1),
		ptIdx.second + ((int)m_vRows.size() / 2 ));

	//4) Впендюриваем всем тайлам индексы текстур
	for (auto pTile : m_vTiles) {
		auto spIdx = pTile->getIndex();
		int nX = spIdx.first + spIdx0.first;
		int nY = spIdx0.second - spIdx.second;

		if ((nX < 0) || (nX > nTileCount) || (nY < 0) || (nY > nTileCount))
			continue;

		pTile->setTileIndex({ nX, nY }, m_uiZoomLevel);
	}

	return true;
}

void CTileMap::move()
{
	if (m_vCols.empty() || m_vRows.empty() || m_vTiles.empty())
		return;
	
	//0) Вычисляем координаты экранного окна
	auto pRender = m_pGlobal.lock();
	auto qvLB = pRender->screenToWorld(0, pRender->getHeight());
	auto qvRT = pRender->screenToWorld(pRender->getWidth(), 0);

	//1) Проверяем выход за границы экрана
	checkLeftBorder(qvRT.x());
	checkRightBorder(qvLB.x());
	checkBottomBorder(qvRT.y());
	checkTopBorder(qvLB.y());
}

void CTileMap::rebuild()
{
	rebuildTileGeometry();
}

IGlobalRendererPtr CTileMap::renderer()
{
	return m_pGlobal.lock();
}

void CTileMap::rebuildTileGeometry()
{
	//0) Для начала - определим размер тайла в пикселах
	auto pMeta = CBingGeoTextureProvider::get()->getMetadata();
	if (!pMeta->valid())
		return;

	auto qsSize = pMeta->getImageSize();
	
	//1) Теперь пересчитаем это в мировые координаты и вычислим геометрические размеры тайла в координатах карты
	auto pRenderer = m_pGlobal.lock();
	auto qvP0 = pRenderer->screenToWorld(0, qsSize.height());
	auto qvP1 = pRenderer->screenToWorld(qsSize.width(), 0);
	m_dbTileWidth = qvP1.x() - qvP0.x();
	m_dbTileHeight = qvP1.y() - qvP0.y();
	QVector3D qvSize = { m_dbTileWidth, m_dbTileHeight, 0.f };
		
	//2) Определяем размер видимой области в координатах экрана и ее центр
	auto qvLB = pRenderer->screenToWorld(0, pRenderer->getHeight());
	auto qvRT = pRenderer->screenToWorld(pRenderer->getWidth(), 0);
	QPointF qpScreenCenter = { qvLB.x() + (qvRT.x() - qvLB.x()) / 2.0, qvLB.y() + (qvRT.y() - qvLB.y()) / 2.0 };

	//3) Центр экрана попадает в середину поля тайлов. Вычисляем координаты нулевого тайла
	QVector3D qvTile0 = { (float)qpScreenCenter.x() - m_vCols.size() * m_dbTileWidth / 2.f,
		(float)qpScreenCenter.y() - m_vRows.size() * m_dbTileHeight / 2.f, 0.f };

	//4) Теперь впендюрим всем тайлам их координаты
	for (auto pTile : m_vTiles) {
		auto spIdx = pTile->getIndex();
		pTile->place({ qvTile0.x() + spIdx.first * m_dbTileWidth,
						qvTile0.y() + spIdx.second * m_dbTileHeight, 0.f }, qvSize);
	}
}

void CTileMap::checkLeftBorder(const float& dbScreenX)
{
	auto pRow = m_vRows[0];
	auto pTile = pRow->rbegin()->lock();

	if (dbScreenX <= pTile->getPos().x())
		return;

	//0) Вычисляем сколько тайлов надо переместить направо
	int nDelta = (dbScreenX - pTile->getPos().x()) / pTile->getSize().x() + 1;
	if (nDelta <= 0)
		return;

	auto pMath = CBingGeoTextureProvider::get()->getMath();
	auto nMaxIndex = pMath->getTileIndexRange(m_uiZoomLevel);
	
	for (auto it : m_vRows) {
		//1) Перестраиваем индексы и сбрасываем текстуры
		*it << nDelta;

		//2) Определяем где находится последний правильный тайл
		auto pTileLast = (it->rbegin() + nDelta)->lock();
		auto qvSize = pTileLast->getSize();
		auto qvPos = pTileLast->getPos();
		auto qpTileIdx = pTileLast->getTileIndex();

		for (int i = 0; i < nDelta; ++i) {
			auto pTile = (it->rbegin() + i)->lock();
			QVector3D qvNewPos = { qvPos.x() + (nDelta - i) * qvSize.x(), qvPos.y(), 0.f };
			pTile->place(qvNewPos, qvSize);

			int nX = qpTileIdx.x() + (nDelta - i);
			if (nX > nMaxIndex)
				continue;

			QPoint qpGeo = { nX, qpTileIdx.y() };
			pTile->setTileIndex(qpGeo, m_uiZoomLevel);
		}
	}
}

void CTileMap::checkRightBorder(const float& dbScreenX)
{
	auto pRow = m_vRows[0];
	auto pTile = pRow->begin()->lock();

	if (dbScreenX >= pTile->getPos().x())
		return;

	//0) Вычисляем сколько тайлов надо переместить налево
	int nDelta = (pTile->getPos().x() - dbScreenX) / pTile->getSize().x() + 1;
	if (nDelta <= 0)
		return;

	auto pMath = CBingGeoTextureProvider::get()->getMath();
	auto nMaxIndex = pMath->getTileIndexRange(m_uiZoomLevel);

	for (auto it : m_vRows) {
		//1) Перестраиваем индексы и сбрасываем текстуры
		*it >> nDelta;

		//2) Определяем где находится последний правильный тайл
		auto pTileLast = (it->begin() + nDelta)->lock();
		auto qvSize = pTileLast->getSize();
		auto qvPos = pTileLast->getPos();
		auto qpTileIdx = pTileLast->getTileIndex();

		for (int i = 0; i < nDelta; ++i) {
			auto pTile = (it->begin() + i)->lock();
			QVector3D qvNewPos = { qvPos.x() - (nDelta - i) * qvSize.x(), qvPos.y(), 0.f };
			pTile->place(qvNewPos, qvSize);

			int nX = qpTileIdx.x() - (nDelta - i);
			if (nX < 0)
				continue;

			QPoint qpGeo = { nX, qpTileIdx.y() };
			pTile->setTileIndex(qpGeo, m_uiZoomLevel);
		}
	}
}

void CTileMap::checkBottomBorder(const float& dbScreenY)
{
	auto pCol = m_vCols[0];
	auto pTile = pCol->rbegin()->lock();

	if (dbScreenY <= pTile->getPos().y())
		return;

	int nDelta = (dbScreenY - pTile->getPos().y()) / pTile->getSize().y() + 1;
	if (nDelta <= 0)
		return;

	auto pMath = CBingGeoTextureProvider::get()->getMath();
	auto nMaxIndex = pMath->getTileIndexRange(m_uiZoomLevel);

	for (auto it : m_vCols) {
		//1) Перестраиваем индексы и сбрасываем текстуры
		*it << nDelta;

		//2) Определяем где находится последний правильный тайл
		auto pTileLast = (it->rbegin() + nDelta)->lock();
		auto qvSize = pTileLast->getSize();
		auto qvPos = pTileLast->getPos();
		auto qpTileIdx = pTileLast->getTileIndex();

		for (int i = 0; i < nDelta; ++i) {
			auto pTile = (it->rbegin() + i)->lock();
			QVector3D qvNewPos = { qvPos.x(), qvPos.y() + (nDelta - i) * qvSize.y(), 0.f };
			pTile->place(qvNewPos, qvSize);

			int nY = qpTileIdx.y() - (nDelta - i);
			if (nY < 0)
				continue;

			QPoint qpGeo = { qpTileIdx.x(), nY };
			pTile->setTileIndex(qpGeo, m_uiZoomLevel);
		}
	}
}

void CTileMap::checkTopBorder(const float& dbScreenY)
{
	auto pCol = m_vCols[0];
	auto pTile = pCol->begin()->lock();

	if (dbScreenY >= pTile->getPos().y())
		return;

	int nDelta = (pTile->getPos().y() - dbScreenY) / pTile->getSize().y() + 1;
	if (nDelta <= 0)
		return;

	auto pMath = CBingGeoTextureProvider::get()->getMath();
	auto nMaxIndex = pMath->getTileIndexRange(m_uiZoomLevel);

	for (auto it : m_vCols) {
		//1) Перестраиваем индексы и сбрасываем текстуры
		*it >> nDelta;

		//2) Определяем где находится последний правильный тайл
		auto pTileLast = (it->begin() + nDelta)->lock();
		auto qvSize = pTileLast->getSize();
		auto qvPos = pTileLast->getPos();
		auto qpTileIdx = pTileLast->getTileIndex();

		for (int i = 0; i < nDelta; ++i) {
			auto pTile = (it->begin() + i)->lock();
			QVector3D qvNewPos = { qvPos.x(), qvPos.y() - (nDelta - i) * qvSize.y(), 0.f };
			pTile->place(qvNewPos, qvSize);

			int nY = qpTileIdx.y() + (nDelta - i);
			if (nY > nMaxIndex)
				continue;

			QPoint qpGeo = { qpTileIdx.x(), nY };
			pTile->setTileIndex(qpGeo, m_uiZoomLevel);
		}
	}
}

