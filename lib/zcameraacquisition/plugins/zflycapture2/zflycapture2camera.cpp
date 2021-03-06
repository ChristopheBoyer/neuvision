//
// Z3D - A structured light 3D scanner
// Copyright (C) 2013-2016 Nicolas Ulrich <nikolaseu@gmail.com>
//
// This file is part of Z3D.
//
// Z3D is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Z3D is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Z3D.  If not, see <http://www.gnu.org/licenses/>.
//

#include "zflycapture2camera.h"

#include <QDebug>
#include <QTime>

namespace Z3D
{

void PrintCameraInfo( FlyCapture2::CameraInfo* pCamInfo )
{
    printf(
                "\n*** CAMERA INFORMATION ***\n"
                "Serial number - %u\n"
                "Camera model - %s\n"
                "Camera vendor - %s\n"
                "Sensor - %s\n"
                "Resolution - %s\n"
                "Firmware version - %s\n"
                "Firmware build time - %s\n\n",
                pCamInfo->serialNumber,
                pCamInfo->modelName,
                pCamInfo->vendorName,
                pCamInfo->sensorInfo,
                pCamInfo->sensorResolution,
                pCamInfo->firmwareVersion,
                pCamInfo->firmwareBuildTime );
}




ZFlyCapture2Camera::ZFlyCapture2Camera(FlyCapture2::Camera* cameraHandle)
    : ZCameraBase()
    , m_pCamera(cameraHandle)
    , m_currentBufferNumber(0)
{
    FlyCapture2::Error error;

    /// Get the camera information
    error = m_pCamera->GetCameraInfo( &m_cameraInfo );
    if( error != FlyCapture2::PGRERROR_OK ) {
        qFatal(error.GetDescription());
    }

    m_uuid = QString("PGR-SN%1").arg(m_cameraInfo.serialNumber);

    /// Connect the camera control dialog to the camera object
    m_camCtlDlg.Connect( m_pCamera );

    /// Configure camera initial resolution
    error = m_pCamera->SetVideoModeAndFrameRate(FlyCapture2::VIDEOMODE_1280x960Y8, FlyCapture2::FRAMERATE_15);
    if (error != FlyCapture2::PGRERROR_OK) {
        qFatal(error.GetDescription());
    }
}

QList<ZCameraPtr> ZFlyCapture2Camera::getConnectedCameras()
{
    QList<ZCameraPtr> cameras;

    FlyCapture2::Error error;
    FlyCapture2::BusManager busMgr;

    unsigned int ncam = 0;
    error = busMgr.GetNumOfCameras(&ncam);
    if ( error != FlyCapture2::PGRERROR_OK ) {
        qCritical(error.GetDescription());
        return cameras;
    }
    qDebug() << "[FlyCapture2Camera] Number of cameras detected:" << ncam;

    FlyCapture2::PGRGuid arGuid[64];

    cameras.reserve(ncam);

    for (int iCam = 0; iCam < ncam; ++iCam) {

        FlyCapture2::Camera* cameraHandle;

        error = busMgr.GetCameraFromIndex(iCam, &arGuid[iCam]);
        if ( error != FlyCapture2::PGRERROR_OK ) {
            qCritical(error.GetDescription());
            continue;
        }

        FlyCapture2::InterfaceType ifType;
        error = busMgr.GetInterfaceTypeFromGuid( &arGuid[iCam], &ifType );
        if ( error != FlyCapture2::PGRERROR_OK ) {
            qCritical(error.GetDescription());
            continue;
        }

        if ( ifType == FlyCapture2::INTERFACE_GIGE ) {
            qCritical("[FlyCapture2Camera] FlyCapture2::GigECamera not implemented yet...");
            //m_pCamera = new FlyCapture2::GigECamera;
            continue;
        } else {
            cameraHandle = new FlyCapture2::Camera;
        }

        // Connect to the 0th selected camera
        error = cameraHandle->Connect( &arGuid[iCam] );
        if( error != FlyCapture2::PGRERROR_OK ) {
            delete cameraHandle;
            qCritical(error.GetDescription());
            continue;
        }

        qDebug() << "[FlyCapture2Camera] Connected to camera" << iCam;

        cameras << Z3D::ZCameraPtr(new ZFlyCapture2Camera(cameraHandle));
    }

    return cameras;
}

ZCameraPtr ZFlyCapture2Camera::getCameraByName(QString name)
{
//    FlyCapture2::Error error;
//    FlyCapture2::BusManager busMgr;

//    busMgr.GetCameraFromIndex();
//    busMgr.GetCameraFromIPAddress();
//    busMgr.GetCameraFromSerialNumber();

    QList<ZCameraPtr> cameraList = getConnectedCameras();
    if (cameraList.size())
        return cameraList.first();

    return nullptr;
}

ZFlyCapture2Camera::~ZFlyCapture2Camera()
{
    if (m_pCamera) {
        m_pCamera->StopCapture();
        m_pCamera->Disconnect();
        delete m_pCamera;
    }
}
/*
void FlyCapture2Camera::setImageSize(cv::Size resolution)
{
    bool wasCapturing = isRunning();

    if (wasCapturing)
        stopCapture();

    {
        // mutex locker only in this section...
        QMutexLocker lock(&m_mutex);

        // update state
        AbstractCamera::setImageSize(resolution);

        // change camera resolution
        FlyCapture2::VideoMode videoMode = FlyCapture2::VIDEOMODE_640x480Y8;
        FlyCapture2::FrameRate frameRate = FlyCapture2::FRAMERATE_30;

        if (resolution.width > 640 || resolution.height > 480) {
            // if more than 640x480, set next resolution
            videoMode = FlyCapture2::VIDEOMODE_1280x960Y8;
            // when 1280x960 the max frame rate is 15
            frameRate = FlyCapture2::FRAMERATE_15;
        }

        // Configure camera
        FlyCapture2::Error error = m_pCamera->SetVideoModeAndFrameRate(videoMode, frameRate);
        if (error != FlyCapture2::PGRERROR_OK) {
            qFatal(error.GetDescription());
        }
    }

    // Start capturing images
    if (wasCapturing)
        startCapture();
}
*/
void ZFlyCapture2Camera::showSettingsDialog()
{
    m_camCtlDlg.Show();
}

void ZFlyCapture2Camera::imageEventCallback(FlyCapture2::Image *pImage, const void *pCallbackData)
{
    //reinterpret_cast<FlyCapture2Camera*>(pCallbackData)->imageEventCallbackInternal(pImage);
    reinterpret_cast<ZFlyCapture2Camera*>((void *)pCallbackData)->imageEventCallbackInternal(pImage);
}

void ZFlyCapture2Camera::imageEventCallbackInternal(FlyCapture2::Image *pImage)
{
    //QMutexLocker locker(&m_mutex);

    //QTime elapsedTime;
    //elapsedTime.start();

    /// make a copy to avoid modifications at mid time
    FlyCapture2::Error error = m_rawImage.DeepCopy(pImage);
    if (error != FlyCapture2::PGRERROR_OK) {
        qCritical() << "[FlyCapture2Camera] ERROR: Can't copy raw image." << error.GetDescription();
        return;
    }

    /// Convert the raw image
    switch (m_rawImage.GetBitsPerPixel()) {
    case 8:
        error = m_rawImage.Convert( FlyCapture2::PIXEL_FORMAT_MONO8, &m_processedImage );
        break;
    case 16:
        error = m_rawImage.Convert( FlyCapture2::PIXEL_FORMAT_MONO16, &m_processedImage );
        break;
    default:
        qWarning() << "m_rawImage.GetBitsPerPixel() = " << m_rawImage.GetBitsPerPixel() << "not implemented!";
    }

    if (error != FlyCapture2::PGRERROR_OK) {
        qCritical() << "[FlyCapture2Camera] ERROR: Can't convert raw image." << error.GetDescription();
        return;
    }

    /// get image from buffer
    ZCameraImagePtr currentImage = getNextBufferImage(m_processedImage.GetCols(), m_processedImage.GetRows(),
                                                           0, 0, //offset x e y
                                                           m_processedImage.GetBitsPerPixel() / 8 ); //1); //bytes per pixel

    /// copy data
    currentImage->setBuffer(m_processedImage.GetData());

    /// set image number
    currentImage->setNumber(m_currentBufferNumber++);

    //qDebug() << m_processedImage.GetBitsPerPixel() << m_processedImage.GetStride() << currentImage->number();

    /// notify
    emit newImageReceived(currentImage);
}

QList<ZCameraInterface::ZCameraAttribute> ZFlyCapture2Camera::getAllAttributes()
{
    //! TODO
    return QList<ZCameraInterface::ZCameraAttribute>();
}

QVariant ZFlyCapture2Camera::getAttribute(const QString &name) const
{
    //! TODO
    return QVariant("INVALID");
}

bool ZFlyCapture2Camera::setAttribute(const QString &name, const QVariant &value, bool notify)
{
    //! TODO
    return false;
}

bool ZFlyCapture2Camera::startAcquisition()
{
    //QMutexLocker lock(&m_mutex);
    if (ZCameraBase::startAcquisition()) {
        m_currentBufferNumber = 0;
        // Start capturing images
        FlyCapture2::Error error = m_pCamera->StartCapture( &ZFlyCapture2Camera::imageEventCallback, this);
        if (error != FlyCapture2::PGRERROR_OK) {
            qFatal(error.GetDescription());
        }
        return true;
    }

    return false;
}

bool ZFlyCapture2Camera::stopAcquisition()
{
    //QMutexLocker lock(&m_mutex);
    if (ZCameraBase::stopAcquisition()) {
        // stop capturing
        FlyCapture2::Error error = m_pCamera->StopCapture();
        if (error != FlyCapture2::PGRERROR_OK) {
            qFatal(error.GetDescription());
        }
        return true;
    }
    return false;
}

} // namespace Z3D
