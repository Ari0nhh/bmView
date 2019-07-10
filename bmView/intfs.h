#pragma once

interface IGlobalRenderer {
	virtual void init() = 0;
	virtual uint getZoomLevel() = 0;
	virtual QPointF getCenter() = 0;
	virtual uint getWidth() = 0;
	virtual uint getHeight() = 0;
	virtual void repaint() = 0;
	virtual QVector3D screenToWorld(const int&, const int&) = 0;
	virtual ~IGlobalRenderer() = default;
};
using IGlobalRendererPtr = std::shared_ptr<IGlobalRenderer>;
using IGlobalRendererPtr_ = std::weak_ptr<IGlobalRenderer>;

interface ITile {
	virtual std::pair<uint, uint> getIndex() = 0;
	virtual void setIndex(const std::pair<uint, uint>&) = 0;
	virtual QPoint getTileIndex() = 0;
	virtual void setTileIndex(const QPoint&, const uint&) = 0;

	virtual QVector3D getPos() = 0;
	virtual QVector3D getSize() = 0;
	
	virtual void initGL() = 0;
	virtual void place(const QVector3D&, const QVector3D&) = 0;
	virtual void draw(const QMatrix4x4&) = 0;
	virtual void invalidate() = 0;
	virtual ~ITile() = default;
};
using ITilePtr = std::shared_ptr<ITile>;
using ITilePtr_ = std::weak_ptr<ITile>;

interface ITileCircularBuffer {
	virtual void addFirst(ITilePtr) = 0;
	virtual void addLast(ITilePtr) = 0;
	virtual ITileCircularBuffer& operator >> (const uint&) = 0;
	virtual ITileCircularBuffer& operator << (const uint&) = 0;
	virtual std::vector<ITilePtr_>::const_iterator begin() = 0;
	virtual std::vector<ITilePtr_>::const_iterator end() = 0;
	virtual std::vector<ITilePtr_>::reverse_iterator rbegin() = 0;
	virtual std::vector<ITilePtr_>::reverse_iterator rend() = 0;
	virtual ITilePtr at(const size_t&) = 0;
	virtual bool empty() = 0;
	virtual size_t size() = 0;
	virtual void rebuild() = 0;
	~ITileCircularBuffer() = default;
};
using ITileCircularBufferPtr = std::shared_ptr<ITileCircularBuffer>;

interface ITileMap {
	virtual void init() = 0;
	virtual void initGL() = 0;
	virtual void draw(const QMatrix4x4&) = 0;
	virtual bool detail(const uint&) = 0;
	virtual void move() = 0;
	virtual void rebuild() = 0;
	virtual IGlobalRendererPtr renderer() = 0;
	virtual ~ITileMap() = default;
};
using ITileMapPtr = std::shared_ptr<ITileMap>;
using ITileMapPtr_ = std::weak_ptr<ITileMap>;

interface IGeoTexture : public QObject {
	virtual void init() = 0;
	virtual bool valid() = 0;
	virtual bool bind() = 0;
	virtual ~IGeoTexture() = default;
};
using IGeoTexturePtr = std::shared_ptr<IGeoTexture>;

interface IGeoMetadata  {
	virtual bool valid() = 0;
	virtual QString getUriTemplate() = 0;
	virtual QSize getImageSize() = 0;
	virtual std::pair<uint, uint> getZoomLevels() = 0;
	virtual ~IGeoMetadata() = default;
};
using IGeoMetadataPtr = std::shared_ptr<IGeoMetadata>;

interface IGeoMath {
	/*Возвращает размер карты в пикселах для выбранного уровня детализации (УД)*/
	virtual uint getMapSize(const uint&) = 0;
	/*Разрешение карты (метров на пиксель) для выбранных широты и уровня детализации */
	virtual double getResolution(const double&, const uint&) = 0;
	/*Масштаб 1:N для широты, УД и DPI*/
	virtual double getScale(const double&, const uint&, const uint&) = 0;
	/*Преобразование координат WGS-84 в пиксели. На вход идет широта, долгота и УД*/
	virtual std::pair<int, int> wgs2pix(const double&, const double&, const uint&) = 0;
	/*Обратное преобразование. Возвращает широту и долготу. На вход X, Y и УД*/
	virtual std::pair<double, double> pix2wgs(const int&, const int&, const uint&) = 0;
	/*Преобразование пиксельных координат в адрес тайла*/
	virtual std::pair<int, int> pix2tile(const int&, const int&) = 0;
	/*Преобразование адреса тайла в координаты левого верхнего угла*/
	virtual std::pair<int, int> tile2pix(const int&, const int&) = 0;
	/*Преобразование координат тайла в QuadKey*/
	virtual QString tile2quad(const int&, const int&, const uint&) = 0;
	/*Преобразование QuadKey в координаты тайла и УД*/
	virtual std::tuple<int, int, uint> quad2tile(const QString&) = 0;
	/*Возвращает число тайлов на заданном уровне детализации*/
	virtual int getTileCount(const uint&) = 0;
	/*Диапазон индексов тайлов для заданного УД*/
	virtual int getTileIndexRange(const uint&) = 0;
	virtual ~IGeoMath() = default;
};
using IGeoMathPtr = std::shared_ptr<IGeoMath>;

using GeoCallback = std::function<void()>;

interface IGeoTextureProvider {
	virtual IGeoMetadataPtr getMetadata() = 0;
	virtual IGeoMathPtr getMath() = 0;
	virtual IGeoTexturePtr getTexture(const QString&, GeoCallback) = 0;
	virtual ~IGeoTextureProvider() = default;
};
using IGeoTextureProviderPtr = std::shared_ptr<IGeoTextureProvider>;


