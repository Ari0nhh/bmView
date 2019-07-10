#include "geotex.h"
#include "consts.h"
#include <math.h>

IGeoTextureProviderPtr CBingGeoTextureProvider::m_pProvider = nullptr;

CBingGeoTexture::CBingGeoTexture(const QString& qsQuadKey, GeoCallback callback) :
	m_qsQuadKey(qsQuadKey), m_fCallback(callback)
{
	connect(this, &CBingGeoTexture::textureReady, this, &CBingGeoTexture::onTextureReady, Qt::QueuedConnection);
}

CBingGeoTexture::~CBingGeoTexture()
{
	m_CTS.cancel();
	disconnect(this, 0, 0, 0);
}

void CBingGeoTexture::init()
{
	tryLoadTexture();
}

bool CBingGeoTexture::valid()
{
	return m_bValid;
}

bool CBingGeoTexture::bind()
{
	if (!m_bValid)
		return false;

	m_pTexture->bind();
	return true;
}

void CBingGeoTexture::onTextureReady(QImage img)
{
	try {
		if (m_Task.get()) {
			bool bDone = m_Task.is_done();
			m_pTexture = std::make_shared<QOpenGLTexture>(img);
			m_bValid = true;
			m_fCallback();
		}
	}
	catch (const std::exception& e) {
		m_bValid = false;
	}
}

void CBingGeoTexture::tryLoadTexture()
{
	auto pMeta = CBingGeoTextureProvider::get()->getMetadata();
	if (!pMeta->valid())
		return;

	auto qsUri = pMeta->getUriTemplate();
	qsUri.replace("{quadkey}", m_qsQuadKey);

	auto token = m_CTS.get_token();

	web::http::client::http_client client(qsUri.toStdWString());
	m_Task = client.request(web::http::methods::GET)
		.then([=](web::http::http_response response) {
			return response.extract_vector();
			})
		.then([=](std::vector<unsigned char> vData) {
				if (token.is_canceled()) {
					pplx::cancel_current_task();
					return false;
				}
				else {
					QByteArray array(reinterpret_cast<const char*>(vData.data()), vData.size());
					QBuffer buffer(&array);
					buffer.open(QIODevice::ReadOnly);

					QImageReader reader(&buffer);
					QImage img(reader.read());
					emit this->textureReady(img.mirrored());
					return true;
				}
			}, token)
		.then([=](pplx::task<bool> prevTask) -> bool {
			try {
				if (token.is_canceled()) {
					pplx::cancel_current_task();
					return false;
				}else
					return prevTask.get();
			}
			catch (const std::exception& e) {
				return false;
			}
		}, token);
}

IGeoTextureProviderPtr CBingGeoTextureProvider::get()
{
	if (!m_pProvider)
		m_pProvider = IGeoTextureProviderPtr(new CBingGeoTextureProvider());

	return m_pProvider;
}

IGeoMetadataPtr CBingGeoTextureProvider::getMetadata()
{
	if (!m_pMetadata)
		m_pMetadata = std::make_shared<CBingGeoMetadata>();

	return m_pMetadata;
}

IGeoMathPtr CBingGeoTextureProvider::getMath()
{
	if (!m_pMath)
		m_pMath = std::make_shared<CBingGeoMath>();

	return m_pMath;
}

IGeoTexturePtr CBingGeoTextureProvider::getTexture(const QString& qsQuadKey, GeoCallback callback)
{
	return std::make_shared<CBingGeoTexture>(qsQuadKey, callback);
}

CBingGeoMetadata::CBingGeoMetadata()
{
	populateData();
}

bool CBingGeoMetadata::valid()
{
	return m_bValid;
}

QString CBingGeoMetadata::getUriTemplate()
{
	return m_qsUriTemplate;
}

QSize CBingGeoMetadata::getImageSize()
{
	return m_qsSize;
}

std::pair<uint, uint> CBingGeoMetadata::getZoomLevels()
{
	return m_upZoom;
}

void CBingGeoMetadata::populateData()
{
	std::wstringstream ss;
	ss << L"http://dev.virtualearth.net/REST/v1/Imagery/Metadata/Aerial?output=json";
	ss << L"&include=ImageryProviders&uriScheme=http&key=" << gsBingAPIKey;
	
	web::http::client::http_client client(ss.str().c_str());
	auto requestTask = client.request(web::http::methods::GET)
	
	.then([=](web::http::http_response response) {
		return response.extract_string(true);
		})

	.then([=](std::wstring wsBody) {
			//2) Parse image metadata json
			QString qsData = QString::fromStdWString(wsBody);
			QJsonDocument qjDoc = QJsonDocument::fromJson(qsData.toUtf8());
			auto qjRoot = qjDoc.object();
			if (qjRoot.isEmpty())
				return;

			auto qjRSets = qjRoot["resourceSets"].toArray();
			if (qjRSets.isEmpty() || (qjRSets.count() < 1))
				return;

			auto qjRSetItem = qjRSets[0].toObject();
			if (qjRSetItem.isEmpty())
				return;

			auto qjResources = qjRSetItem["resources"].toArray();
			if (qjResources.isEmpty() || (qjResources.count() < 1))
				return;

			auto qjRes = qjResources[0].toObject();
			if (qjRes.isEmpty())
				return;

			//Size
			auto nHeight = qjRes["imageHeight"].toInt();
			auto nWidth = qjRes["imageWidth"].toInt();
			m_qsSize = { nWidth, nHeight };

			//Zoom
			auto nMin = qjRes["zoomMin"].toInt();
			auto nMax = qjRes["zoomMax"].toInt();
			m_upZoom = std::make_pair(nMin, nMax);

			//Uri
			m_qsUriTemplate = qjRes["imageUrl"].toString();
			auto qjSubDomains = qjRes["imageUrlSubdomains"].toArray();
			if (qjSubDomains.isEmpty() || (qjSubDomains.count() < 1))
				return;

			auto qsSubDomain = qjSubDomains[0].toString();
			m_qsUriTemplate.replace("{subdomain}", qsSubDomain);
			m_bValid = true;
	});

	try {
		requestTask.wait();
	}
	catch (const std::exception& e) {
		m_bValid = false;
	}
}



uint CBingGeoMath::getMapSize(const uint& uiZoomLevel)
{
	return 256 << uiZoomLevel;
}

double CBingGeoMath::getResolution(const double& dbLattitude, const uint& uiZoomLevel)
{
	auto dbLat = clip(dbLattitude, m_dbMinLattitude, m_dbMaxLattitude);
	return std::cos(dbLat * M_PI / 180.0) * 2 * M_PI * m_dbEathRadius / getMapSize(uiZoomLevel);
}

double CBingGeoMath::getScale(const double& dbLattitude, const uint& uiZoomLevel, const uint& uiDPI)
{
	return getResolution(dbLattitude, uiZoomLevel) * uiDPI / 0.0254;
}

std::pair<int, int> CBingGeoMath::wgs2pix(const double& dbLattitude, const double& dbLongitude, const uint& uiZoomLevel)
{
	auto dbLat = clip(dbLattitude, m_dbMinLattitude, m_dbMaxLattitude);
	auto dbLng = clip(dbLongitude, m_dbMinLongitude, m_dbMaxLongitude);

	double dbX = (dbLng + 180.0) / 360.0;
	double dbSinLat = std::sin(dbLat * M_PI / 180.0);
	double dbY = 0.5 - std::log((1.0 + dbSinLat) / (1 - dbSinLat)) / (4 * M_PI);

	auto uiSize = getMapSize(uiZoomLevel);
	int nX = clip((int)(dbX * uiSize + 0.5), 0, (int)(uiSize - 1));
	int nY = clip((int)(dbY * uiSize + 0.5), 0, (int)(uiSize - 1));

	return std::make_pair(nX, nY);
}

std::pair<double, double> CBingGeoMath::pix2wgs(const int& nX, const int& nY, const uint& uiZoomLevel)
{
	auto uiMapSize = getMapSize(uiZoomLevel);
	double dbX = clip(nX, 0, (int)(uiMapSize - 1)) / uiMapSize - 0.5;
	double dbY = 0.5 - clip(nY, 0, (int)(uiMapSize - 1)) / uiMapSize;

	double dbLat = 90.0 - 360.0 * std::atan(std::exp(-2.0 * dbY * M_PI)) / M_PI;
	double dbLong = 360.0 * dbX;

	return std::make_pair(dbLat, dbLong);
}

std::pair<int, int> CBingGeoMath::pix2tile(const int& nX, const int& nY)
{
	return std::make_pair(nX / 256, nY / 256);
}

std::pair<int, int> CBingGeoMath::tile2pix(const int& nX, const int& nY)
{
	return std::make_pair(256 * nX, 256 * nY);
}

QString CBingGeoMath::tile2quad(const int& nX, const int& nY, const uint& uiZoomLevel)
{
	std::string sResult;
	for (auto i = uiZoomLevel; i > 0; --i) {
		char cDigit = '0';
		int nMask = 1 << (i - 1);
		
		if (0 != (nX & nMask))
			++cDigit;

		if (0 != (nY & nMask))
			cDigit += 2;

		sResult += cDigit;
	}

	return QString::fromStdString(sResult);
}

std::tuple<int, int, uint> CBingGeoMath::quad2tile(const QString& qsKey)
{
	uint uiZoom = qsKey.length();
	int nX = 0, nY = 0;
	for (auto i = uiZoom; i > 0; --i) {
		int nMask = 1 << (i - 1);
		
		if ('1' == qsKey[i]) {
			nX |= nMask;
		}

		if ('2' == qsKey[i]) {
			nY |= nMask;
		}

		if ('3' == qsKey[i]) {
			nX |= nMask;
			nY |= nMask;
		}
	}

	return std::make_tuple(nX, nY, uiZoom);
}

int CBingGeoMath::getTileCount(const uint& uiZoomLevel)
{
	return std::pow(2, 2 * uiZoomLevel);
}

int CBingGeoMath::getTileIndexRange(const uint& uiZoomLevel)
{
	return std::pow(2, uiZoomLevel) - 1;
}
