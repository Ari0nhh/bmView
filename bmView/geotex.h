#pragma once
#include "intfs.h"

class CBingGeoTexture : public IGeoTexture {
	Q_OBJECT
signals:
	void textureReady(QImage img);
public:
	explicit CBingGeoTexture(const QString& qsQuadKey, GeoCallback callback);
	~CBingGeoTexture();
protected: //IGeoTexture
	void init() override;
	bool valid() override;
	bool bind() override;
protected slots:
	void onTextureReady(QImage img);
private:
	std::shared_ptr<QOpenGLTexture> m_pTexture = nullptr;
	pplx::cancellation_token_source m_CTS;
	bool m_bValid = false;
	QString m_qsQuadKey;
	GeoCallback m_fCallback;
	pplx::task<bool> m_Task;

	void tryLoadTexture();
};

class CBingGeoMetadata : public IGeoMetadata {
public:
	explicit CBingGeoMetadata();
protected: //IGeoMetadata
	bool valid() override;
	QString getUriTemplate() override;
	QSize getImageSize() override;
	std::pair<uint, uint> getZoomLevels() override;
private:
	bool m_bValid = false;
	QString m_qsUriTemplate;
	QSize m_qsSize;
	std::pair<uint, uint> m_upZoom;
	void populateData();
};

class CBingGeoMath : public IGeoMath {
protected: //IGeoMath
	uint getMapSize(const uint& uiZoomLevel) override;
	double getResolution(const double& dbLattitude, const uint& uiZoomLevel) override;
	double getScale(const double& dbLattitude, const uint& uiZoomLevel, const uint& uiDPI) override;
	std::pair<int, int> wgs2pix(const double& dbLattitude, const double& dbLongitude, const uint& uiZoomLevel) override;
	std::pair<double, double> pix2wgs(const int& nX, const int& nY, const uint& uiZoomLevel) override;
	std::pair<int, int> pix2tile(const int& nX, const int& nY) override;
	std::pair<int, int> tile2pix(const int& nX, const int& nY) override;
	QString tile2quad(const int& nX, const int& nY, const uint& uiZoomLevel) override;
	std::tuple<int, int, uint> quad2tile(const QString& qsKey) override;
	int getTileCount(const uint& uiZoomLevel) override;
	int getTileIndexRange(const uint& uiZoomLevel) override;
private: //Consts
	const double m_dbEathRadius	  = 6378137.0;
	const double m_dbMinLattitude = -85.05112878;
	const double m_dbMaxLattitude = 85.05112878;
	const double m_dbMinLongitude = -180.0;
	const double m_dbMaxLongitude = 180.0;
private: //Helpers
	template <typename T> T clip(const T tVal, const T tMin, const T tMax);
};

template <typename T> T CBingGeoMath::clip(const T tVal, const T tMin, const T tMax)
{
	return std::min(std::max(tVal, tMin), tMax);
}

class CBingGeoTextureProvider : public IGeoTextureProvider {
public:
	static IGeoTextureProviderPtr get();
protected: //IGeoTextureProvider
	IGeoMetadataPtr getMetadata() override;
	IGeoMathPtr getMath() override;
	IGeoTexturePtr getTexture(const QString& qsQuadKey, GeoCallback callback) override;
private:
	CBingGeoTextureProvider() = default;
	static IGeoTextureProviderPtr m_pProvider;
	IGeoMetadataPtr m_pMetadata = nullptr;
	IGeoMathPtr m_pMath = nullptr;
};
