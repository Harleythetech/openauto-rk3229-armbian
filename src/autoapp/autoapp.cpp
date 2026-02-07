/*
*  This file is part of openauto project.
*  Copyright (C) 2018 f1x.studio (Michal Szwaj)
*
*  openauto is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 3 of the License, or
*  (at your option) any later version.

*  openauto is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with openauto. If not, see <http://www.gnu.org/licenses/>.
*/

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QScreen>
#include <QQuickStyle>
#include <QCursor>
#include <aasdk/TCP/TCPWrapper.hpp>
#include <aasdk/USB/AccessoryModeQueryChain.hpp>
#include <aasdk/USB/AccessoryModeQueryChainFactory.hpp>
#include <aasdk/USB/AccessoryModeQueryFactory.hpp>
#include <aasdk/USB/ConnectedAccessoriesEnumerator.hpp>
#include <aasdk/USB/USBHub.hpp>
#include <boost/log/utility/setup.hpp>
#include <f1x/openauto/Common/Log.hpp>
#include <f1x/openauto/autoapp/App.hpp>
#include <f1x/openauto/autoapp/Configuration/Configuration.hpp>
#include <f1x/openauto/autoapp/Configuration/IConfiguration.hpp>
#include <f1x/openauto/autoapp/Configuration/RecentAddressesList.hpp>
#include <f1x/openauto/autoapp/Service/AndroidAutoEntityFactory.hpp>
#include <f1x/openauto/autoapp/Service/ServiceFactory.hpp>
#include <f1x/openauto/autoapp/UI/UIBackend.hpp>
#include <f1x/openauto/autoapp/Player/AudioPlayer.hpp>
#include <f1x/openauto/autoapp/Player/FileBrowserBackend.hpp>
#include <thread>

namespace autoapp = f1x::openauto::autoapp;
using ThreadPool = std::vector<std::thread>;

void setOpenAutoEnvironmentDefaults()
{
  auto setIfUnset = [](const char *name, const char *value)
  {
    if (!qEnvironmentVariableIsSet(name))
    {
      qputenv(name, value);
    }
  };

  // Qt eglfs mode for DRM/KMS (FFmpeg takes DRM master for video on plane 31)
  setIfUnset("QT_QPA_PLATFORM", "eglfs");
  setIfUnset("QT_QPA_EGLFS_INTEGRATION", "eglfs_kms");
  setIfUnset("QT_QPA_EGLFS_KMS_ATOMIC", "1");
  setIfUnset("QT_QPA_EGLFS_KMS_CONFIG", "/etc/eglfs.json");

  // Audio configuration for ALSA/RtAudio
  setIfUnset("ALSA_CARD", "0");
  setIfUnset("ALSA_PCM_CARD", "0");
  setIfUnset("RTAUDIO_ALSA_DEVICE", "hw:0,0");

  OPENAUTO_LOG(info) << "[AutoApp] Launching OpenAuto (eglfs + FFmpeg DRM master takeover)...";
  OPENAUTO_LOG(info) << "[AutoApp] Dev: Harleythetech";
  OPENAUTO_LOG(info)
      << "[AutoApp] Git: "
         "https://github.com/Harleythetech/openauto-rk3229-armbian";
}

void startUSBWorkers(boost::asio::io_service &ioService,
                     libusb_context *usbContext, ThreadPool &threadPool)
{
  auto usbWorker = [&ioService, usbContext]()
  {
    timeval libusbEventTimeout{180, 0};

    while (!ioService.stopped())
    {
      libusb_handle_events_timeout_completed(usbContext, &libusbEventTimeout,
                                             nullptr);
    }
  };

  threadPool.emplace_back(usbWorker);
  threadPool.emplace_back(usbWorker);
  threadPool.emplace_back(usbWorker);
  threadPool.emplace_back(usbWorker);
}

void startIOServiceWorkers(boost::asio::io_service &ioService,
                           ThreadPool &threadPool)
{
  auto ioServiceWorker = [&ioService]()
  { ioService.run(); };

  threadPool.emplace_back(ioServiceWorker);
  threadPool.emplace_back(ioServiceWorker);
  threadPool.emplace_back(ioServiceWorker);
  threadPool.emplace_back(ioServiceWorker);
}

void configureLogging()
{
  const std::string logIni = "openauto-logs.ini";
  std::ifstream logSettings(logIni);
  if (logSettings.good())
  {
    try
    {
      // For boost < 1.71 the severity types are not automatically parsed so
      // lets register them.
      boost::log::register_simple_filter_factory<
          boost::log::trivial::severity_level>("Severity");
      boost::log::register_simple_formatter_factory<
          boost::log::trivial::severity_level, char>("Severity");
      boost::log::init_from_stream(logSettings);
    }
    catch (std::exception const &e)
    {
      OPENAUTO_LOG(warning)
          << "[OpenAuto] " << logIni << " was provided but was not valid.";
    }
  }
}

int main(int argc, char *argv[])
{
  configureLogging();
  setOpenAutoEnvironmentDefaults();

  OPENAUTO_LOG(info) << "[AutoApp] Starting OpenAuto with QML UI...";

  libusb_context *usbContext;
  if (libusb_init(&usbContext) != 0)
  {
    OPENAUTO_LOG(error) << "[AutoApp] libusb_init failed.";
    return 1;
  }

  boost::asio::io_service ioService;
  boost::asio::io_service::work work(ioService);
  std::vector<std::thread> threadPool;
  startUSBWorkers(ioService, usbContext, threadPool);
  startIOServiceWorkers(ioService, threadPool);

  // Use QGuiApplication for QML-based UI
  QGuiApplication qApplication(argc, argv);

  // Set Qt Quick Controls 2 style
  QQuickStyle::setStyle("Basic");

  QScreen *primaryScreen = QGuiApplication::primaryScreen();
  int width = 800;
  int height = 480;

  if (primaryScreen)
  {
    QRect screenGeometry = primaryScreen->geometry();
    width = screenGeometry.width();
    height = screenGeometry.height();
    OPENAUTO_LOG(info) << "[AutoApp] Using geometry from primary screen.";
  }
  else
  {
    OPENAUTO_LOG(info)
        << "[AutoApp] Unable to find primary screen, using default values.";
  }

  OPENAUTO_LOG(info) << "[AutoApp] Display width: " << width;
  OPENAUTO_LOG(info) << "[AutoApp] Display height: " << height;

  auto configuration =
      std::make_shared<autoapp::configuration::Configuration>();

  // Hide cursor if configured
  if (configuration->showCursor() == false)
  {
    qApplication.setOverrideCursor(QCursor(Qt::BlankCursor));
  }
  else
  {
    qApplication.setOverrideCursor(QCursor(Qt::ArrowCursor));
  }

  // Create UI backend for QML
  auto uiBackend = new autoapp::ui::UIBackend(configuration);

  // Create music player and file browser
  auto audioPlayer = new autoapp::player::AudioPlayer();
  auto fileBrowser = new autoapp::player::FileBrowserBackend();

  // Connect audioPlayer state to UIBackend music properties
  QObject::connect(audioPlayer, &autoapp::player::AudioPlayer::trackChanged,
                   [uiBackend, audioPlayer]()
                   {
                     uiBackend->setTrackTitle(audioPlayer->trackTitle());
                     uiBackend->setAlbumName(audioPlayer->albumName());
                     uiBackend->setArtistName(audioPlayer->artistName());
                     uiBackend->setAlbumArtPath(audioPlayer->albumArtPath());
                   });
  QObject::connect(audioPlayer, &autoapp::player::AudioPlayer::playbackStateChanged,
                   [uiBackend, audioPlayer]()
                   {
                     uiBackend->setIsPlaying(audioPlayer->isPlaying());
                   });

  // Create QML engine
  QQmlApplicationEngine engine;

  // Expose backend to QML
  engine.rootContext()->setContextProperty("backend", uiBackend);
  engine.rootContext()->setContextProperty("audioPlayer", audioPlayer);
  engine.rootContext()->setContextProperty("fileBrowser", fileBrowser);
  engine.rootContext()->setContextProperty("screenWidth", width);
  engine.rootContext()->setContextProperty("screenHeight", height);

  // Add import path for custom QML modules
  engine.addImportPath("qrc:/qml");

  // Load main QML file
  const QUrl url("qrc:/qml/Main.qml");
  QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, &qApplication, [url](QObject *obj, const QUrl &objUrl)
                   {
    if (!obj && url == objUrl)
    {
      OPENAUTO_LOG(error) << "[AutoApp] Failed to load QML UI.";
      QCoreApplication::exit(-1);
    } }, Qt::QueuedConnection);

  engine.load(url);

  OPENAUTO_LOG(info) << "[AutoApp] QML UI loaded successfully.";

  // Create USB/WiFi Android Auto infrastructure
  aasdk::tcp::TCPWrapper tcpWrapper;
  aasdk::usb::USBWrapper usbWrapper(usbContext);
  aasdk::usb::AccessoryModeQueryFactory queryFactory(usbWrapper, ioService);
  aasdk::usb::AccessoryModeQueryChainFactory queryChainFactory(
      usbWrapper, ioService, queryFactory);
  autoapp::service::ServiceFactory serviceFactory(ioService, configuration);
  autoapp::service::AndroidAutoEntityFactory androidAutoEntityFactory(
      ioService, configuration, serviceFactory);

  auto usbHub(std::make_shared<aasdk::usb::USBHub>(usbWrapper, ioService,
                                                   queryChainFactory));
  auto connectedAccessoriesEnumerator(
      std::make_shared<aasdk::usb::ConnectedAccessoriesEnumerator>(
          usbWrapper, ioService, queryChainFactory));
  auto app = std::make_shared<autoapp::App>(
      ioService, usbWrapper, tcpWrapper, androidAutoEntityFactory,
      std::move(usbHub), std::move(connectedAccessoriesEnumerator));

  // Connect UIBackend signals to Android Auto functionality
  QObject::connect(uiBackend, &autoapp::ui::UIBackend::requestAndroidAuto,
                   [&app](bool usb)
                   {
                     OPENAUTO_LOG(debug) << "[AutoApp] Triggering Android Auto start via "
                                         << (usb ? "USB" : "WiFi");
                     try
                     {
                       app->disableAutostartEntity = false;
                       app->resume();
                       if (usb)
                       {
                         app->waitForUSBDevice();
                       }
                     }
                     catch (...)
                     {
                       OPENAUTO_LOG(error) << "[AutoApp] Exception starting Android Auto.";
                     }
                   });

  QObject::connect(uiBackend, &autoapp::ui::UIBackend::exitRequested,
                   [&qApplication]()
                   {
                     OPENAUTO_LOG(info) << "[AutoApp] Exit requested from UI.";
                     system("touch /tmp/shutdown");
                     qApplication.quit();
                   });

  // Start waiting for USB device
  app->waitForUSBDevice();

  auto result = qApplication.exec();

  // Cleanup
  std::for_each(threadPool.begin(), threadPool.end(),
                std::bind(&std::thread::join, std::placeholders::_1));

  delete fileBrowser;
  delete audioPlayer;
  delete uiBackend;
  libusb_exit(usbContext);
  return result;
}
