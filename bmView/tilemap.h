#pragma once
#include "intfs.h"

class CTile : public ITile {
public:
	explicit CTile(ITileMapPtr pMap);
protected: //ITile
	std::pair<uint, uint> getIndex() override;
	void setIndex(const std::pair<uint, uint>& spIndex) override;
	QPoint getTileIndex() override;
	void setTileIndex(const QPoint& qpIndex, const uint& uiZoom) override;

	QVector3D getPos() override;
	QVector3D getSize() override;
	
	void initGL() override;
	void place(const QVector3D& qvPos, const QVector3D& qvSize) override;
	void draw(const QMatrix4x4& qmWorld) override;
	void invalidate() override;
private:
	bool m_bInvalidate = false;
	std::pair<uint, uint> m_spIndex;
	QVector3D m_qvPos, m_qvSize;
	QPoint m_qpTileIndex;
	uint m_uiZoomLevel;
	ITileMapPtr_ m_pMap;
	IGeoTexturePtr m_pTexture = nullptr;
	IGeoTexturePtr m_pPrevTexture = nullptr;
	
	std::shared_ptr<QOpenGLVertexArrayObject> m_pVAO;
	std::shared_ptr <QOpenGLBuffer> m_pVBO, m_pEBO;
	std::shared_ptr<QOpenGLShaderProgram> m_pShaders;
	int m_nWorldMatrixLoc, m_nPosLoc, m_nSizeLoc;
private:
	bool InitGLBuffers();
	bool InitShaders();
	void onTextureReady();
private:
	std::shared_ptr<GLuint[]>  InitIndexBuffer();
	std::shared_ptr<GLfloat[]>  InitVertexBuffer();
	void invalidateTexture();
};

class CTileCircularBuffer : public ITileCircularBuffer {
public:
	explicit CTileCircularBuffer(bool bVertical) : m_bVertical(bVertical) {};
protected: //ITileCircularBuffer
	void addFirst(ITilePtr pTile) override;
	void addLast(ITilePtr pTile) override;
	ITileCircularBuffer& operator >> (const uint& uiCount) override;
	ITileCircularBuffer& operator << (const uint& uiCount) override;
	std::vector<ITilePtr_>::const_iterator begin() override;
	std::vector<ITilePtr_>::const_iterator end() override;
	std::vector<ITilePtr_>::reverse_iterator rbegin() override;
	std::vector<ITilePtr_>::reverse_iterator rend() override;
	ITilePtr at(const size_t& szIdx) override;
	bool empty() override;
	size_t size() override;
	void rebuild() override;
private:
	std::vector<ITilePtr_> m_vTiles;
	bool m_bVertical;
};

class CTileMap : public ITileMap, public std::enable_shared_from_this<CTileMap> {
public:
	explicit CTileMap(IGlobalRendererPtr pRenderer);
protected: //ITileMap
	void init() override;
	void initGL() override;
	void draw(const QMatrix4x4& qmWorld) override;
	bool detail(const uint& uiZoomLevel) override;
	void move() override;
	void rebuild() override;
	IGlobalRendererPtr renderer() override;
private:
	std::vector<ITileCircularBufferPtr> m_vCols;
	std::vector<ITileCircularBufferPtr> m_vRows;
	std::vector<ITilePtr> m_vTiles;
	float m_dbTileWidth = 0.0; 
	float m_dbTileHeight = 0.0;
	IGlobalRendererPtr_ m_pGlobal;
	uint m_uiZoomLevel = 1u;
	std::pair<uint, uint> m_upZoomLevels;	
	void rebuildTileGeometry();
private:
	void checkLeftBorder(const float& dbScreenX);
	void checkRightBorder(const float& dbScreenX);
	void checkBottomBorder(const float& dbScreenY);
	void checkTopBorder(const float& dbScreenY);
};

