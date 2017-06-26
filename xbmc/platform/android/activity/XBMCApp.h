#pragma once
/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"

#include <math.h>
#include <pthread.h>
#include <string>
#include <vector>
#include <map>
#include <queue>
#include <memory>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <android/native_activity.h>

#include <androidjni/Activity.h>
#include <androidjni/AudioManager.h>
#include <androidjni/AudioDeviceInfo.h>
#include <androidjni/BroadcastReceiver.h>
#include <androidjni/Image.h>
#include <androidjni/SurfaceHolder.h>
#include <androidjni/View.h>

#include "threads/Event.h"
#include "interfaces/IAnnouncer.h"

#include "guilib/Geometry.h"
#include "IActivityHandler.h"
#include "IInputHandler.h"
#include "JNIMainActivity.h"
#include "JNIXBMCAudioManagerOnAudioFocusChangeListener.h"
#include "JNIXBMCMainView.h"
#include "JNIXBMCMediaSession.h"
#include "platform/xbmc.h"

// forward delares
class CJNIWakeLock;
class CAESinkAUDIOTRACK;
class CVariant;
class IInputDeviceCallbacks;
class IInputDeviceEventHandler;
class CVideoSyncAndroid;
typedef struct _JNIEnv JNIEnv;

struct androidIcon
{
  unsigned int width;
  unsigned int height;
  void *pixels;
};

struct androidPackage
{
  std::string packageName;
  std::string className;
  std::string packageLabel;
  int icon;
};

class CActivityResultEvent : public CEvent
{
public:
  CActivityResultEvent(int requestcode)
    : m_requestcode(requestcode)
  {}
  int GetRequestCode() const { return m_requestcode; }
  int GetResultCode() const { return m_resultcode; }
  void SetResultCode(int resultcode) { m_resultcode = resultcode; }
  CJNIIntent GetResultData() const { return m_resultdata; }
  void SetResultData(const CJNIIntent &resultdata) { m_resultdata = resultdata; }

protected:
  int m_requestcode;
  CJNIIntent m_resultdata;
  int m_resultcode;
};

class CCaptureEvent : public CEvent
{
public:
  CCaptureEvent() {}
  jni::CJNIImage GetImage() const { return m_image; }
  void SetImage(const jni::CJNIImage &image) { m_image = image; }

protected:
  jni::CJNIImage m_image;
};

class CXBMCApp
    : public IActivityHandler
    , public CJNIMainActivity
    , public CJNIBroadcastReceiver
    , public ANNOUNCEMENT::IAnnouncer
    , public CJNISurfaceHolderCallback
{
public:
  CXBMCApp(ANativeActivity *nativeActivity);
  virtual ~CXBMCApp();
  static CXBMCApp* get() { return m_xbmcappinstance; }

  // IAnnouncer IF
  virtual void Announce(ANNOUNCEMENT::AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data);

  virtual void onReceive(CJNIIntent intent);
  virtual void onNewIntent(CJNIIntent intent);
  virtual void onActivityResult(int requestCode, int resultCode, CJNIIntent resultData);
  virtual void onCaptureAvailable(jni::CJNIImage image);
  virtual void onScreenshotAvailable(jni::CJNIImage image);
  virtual void onVolumeChanged(int volume);
  virtual void onAudioFocusChange(int focusChange);
  virtual void doFrame(int64_t frameTimeNanos);
  virtual void onVisibleBehindCanceled();
  virtual void onMultiWindowModeChanged(bool isInMultiWindowMode);
  virtual void onPictureInPictureModeChanged(bool isInPictureInPictureMode);
  virtual void onAudioDeviceAdded(CJNIAudioDeviceInfos devices);
  virtual void onAudioDeviceRemoved(CJNIAudioDeviceInfos devices);

  // implementation of CJNIInputManagerInputDeviceListener
  void onInputDeviceAdded(int deviceId) override;
  void onInputDeviceChanged(int deviceId) override;
  void onInputDeviceRemoved(int deviceId) override;

  bool isValid() { return m_activity != NULL; }

  void onStart();
  void onResume();
  void onPause();
  void onStop();
  void onDestroy();

  void onSaveState(void **data, size_t *size);
  void onConfigurationChanged();
  void onLowMemory();

  void onCreateWindow(ANativeWindow* window);
  void onResizeWindow();
  void onDestroyWindow();
  void onGainFocus();
  void onLostFocus();

  void Initialize();
  void Deinitialize();

  static const ANativeWindow** GetNativeWindow(int timeout);
  static int SetBuffersGeometry(int width, int height);
  static int android_printf(const char *format, ...);
  static void BringToFront();
  static GLuint pullTexture();
  static void pushTexture(GLuint tex);

  static int GetBatteryLevel();
  static bool EnableWakeLock(bool on);
  static bool HasFocus() { return m_hasFocus; }
  static bool IsResumed() { return m_isResumed; }
  static void CheckHeadsetPlugged();
  static bool IsHeadsetPlugged();
  static bool IsHDMIPlugged();

  static bool StartAppActivity(const std::string &package, const std::string &cls);
  static bool StartActivity(const std::string &package, const std::string &intent = std::string(), const std::string &dataType = std::string(), const std::string &dataURI = std::string());
  static std::vector <androidPackage> GetApplications();

  /*!
   * \brief If external storage is available, it returns the path for the external storage (for the specified type)
   * \param path will contain the path of the external storage (for the specified type)
   * \param type optional type. Possible values are "", "files", "music", "videos", "pictures", "photos, "downloads"
   * \return true if external storage is available and a valid path has been stored in the path parameter
   */
  static bool GetExternalStorage(std::string &path, const std::string &type = "");
  static bool GetStorageUsage(const std::string &path, std::string &usage);
  static int GetMaxSystemVolume();
  static float GetSystemVolume();
  static void SetSystemVolume(float percent);
  static void InitDirectories();

  static void SetRefreshRate(float rate);
  static void SetDisplayMode(int mode);
  static int GetDPI();

  static CRect MapRenderToDroid(const CRect& srcRect);
  static CPoint GetDroidToGuiRatio();

  static int WaitForActivityResult(const CJNIIntent &intent, int requestCode, CJNIIntent& result);
  static bool WaitForCapture(jni::CJNIImage& image);
  static bool GetCapture(jni::CJNIImage& img);
  static void TakeScreenshot();
  static void StopCapture();

  // Playback callbacks
  void OnPlayBackStarted();
  void OnPlayBackPaused();
  void OnPlayBackStopped();

  // Info callback
  void UpdateSessionMetadata();
  void UpdateSessionState();

  // input device methods
  static void RegisterInputDeviceCallbacks(IInputDeviceCallbacks* handler);
  static void UnregisterInputDeviceCallbacks();
  static const CJNIViewInputDevice GetInputDevice(int deviceId);
  static std::vector<int> GetInputDeviceIds();

  static void RegisterInputDeviceEventHandler(IInputDeviceEventHandler* handler);
  static void UnregisterInputDeviceEventHandler();
  static bool onInputDeviceEvent(const AInputEvent* event);

  // Application slow ping
  void ProcessSlow();

  //PIP
  static void RequestPictureInPictureMode();

  static bool WaitVSync(unsigned int milliSeconds);
  static uint64_t GetVsyncTime() { return m_vsynctime; }

  bool getVideosurfaceInUse();
  void setVideosurfaceInUse(bool videosurfaceInUse);

protected:
  // limit who can access Volume
  friend class CAESinkAUDIOTRACK;

  static int GetMaxSystemVolume(JNIEnv *env);
  bool AcquireAudioFocus();
  bool ReleaseAudioFocus();
  static void RequestVisibleBehind(bool requested);

private:
  static CXBMCApp* m_xbmcappinstance;
  CJNIXBMCAudioManagerOnAudioFocusChangeListener m_audioFocusListener;
  static std::unique_ptr<CJNIXBMCMainView> m_mainView;
  std::unique_ptr<jni::CJNIXBMCMediaSession> m_mediaSession;
  static bool HasLaunchIntent(const std::string &package);
  std::string GetFilenameFromIntent(const CJNIIntent &intent);
  void run();
  void stop();
  void SetupEnv();
  static void SetRefreshRateCallback(CVariant *rate);
  static void SetDisplayModeCallback(CVariant *mode);
  static ANativeActivity *m_activity;
  static CJNIWakeLock *m_wakeLock;
  static int m_batteryLevel;
  static bool m_hasFocus;
  static bool m_isResumed;
  static bool m_hasAudioFocus;
  static bool m_headsetPlugged;
  static bool m_hdmiPlugged;
  static IInputDeviceCallbacks* m_inputDeviceCallbacks;
  static IInputDeviceEventHandler* m_inputDeviceEventHandler;
  static bool m_hasReqVisible;
  static bool m_hasPIP;
  bool m_videosurfaceInUse;
  bool m_firstrun;
  bool m_exiting;
  pthread_t m_thread;
  static CCriticalSection m_applicationsMutex;
  static std::vector<androidPackage> m_applications;
  static std::vector<CActivityResultEvent*> m_activityResultEvents;

  static CCriticalSection m_captureMutex;
  static CCaptureEvent m_captureEvent;
  static std::queue<jni::CJNIImage> m_captureQueue;

  static ANativeWindow* m_window;
  static std::vector<GLuint> m_texturePool;

  static uint64_t m_vsynctime;
  static CEvent m_vsyncEvent;

  void XBMC_DestroyDisplay();
  void XBMC_SetupDisplay();

  static uint32_t m_playback_state;
  static CRect m_surface_rect;

  // CJNISurfaceHolderCallback interface
public:
  void surfaceChanged(CJNISurfaceHolder holder, int format, int width, int height) override;
  void surfaceCreated(CJNISurfaceHolder holder) override;
  void surfaceDestroyed(CJNISurfaceHolder holder) override;
};
