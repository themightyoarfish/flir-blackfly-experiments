#include "SpinGenApi/SpinnakerGenApi.h"
#include "Spinnaker.h"
#include <chrono>
#include <string>
#include <thread>

using std::string;
using namespace Spinnaker::GenApi;

Spinnaker::CameraPtr camera;

void configureChunkData(INodeMap &nodeMap) {
  try {
    CBooleanPtr ptrChunkModeActive = nodeMap.GetNode("ChunkModeActive");
    if (!IsAvailable(ptrChunkModeActive) || !IsWritable(ptrChunkModeActive)) {
      std::cout
          << "SpinnakerCamera configureChunkData():Unable to activate chunk "
             "mode. Aborting..."
          << std::endl;
      throw std::invalid_argument("Unable to set node: ChunkModeActive");
    }
    ptrChunkModeActive->SetValue(true);
    // --- Chunk data types
    NodeList_t entries;
    // Retrieve the selector node
    CEnumerationPtr ptrChunkSelector = nodeMap.GetNode("ChunkSelector");
    if (!IsAvailable(ptrChunkSelector) || !IsReadable(ptrChunkSelector)) {
      std::cout
          << "SpinnakerCamera configureChunkData():Unable to retrieve chunk "
             "selector. Aborting...."
          << std::endl;
      ;
      throw std::invalid_argument("Unable to read node: ChunkSelector");
    }
    // Retrieve entries
    ptrChunkSelector->GetEntries(entries);
    for (size_t i = 0; i < entries.size(); i++) {
      // Select entry to be enabled
      CEnumEntryPtr ptrChunkSelectorEntry = entries.at(i);
      // Go to next node if problem occurs
      if (!IsAvailable(ptrChunkSelectorEntry) ||
          !IsReadable(ptrChunkSelectorEntry)) {
        continue;
      }
      if (ptrChunkSelectorEntry->GetSymbolic() != "ExposureTime")
        continue;
      std::cout << "Enabling entry " << ptrChunkSelectorEntry->GetSymbolic()
                << std::endl;
      ptrChunkSelector->SetIntValue(ptrChunkSelectorEntry->GetValue());

      // Retrieve corresponding boolean
      CBooleanPtr ptrChunkEnable = nodeMap.GetNode("ChunkEnable");
      // Enable the boolean, thus enabling the corresponding chunk data
      if (!IsAvailable(ptrChunkEnable) && !IsWritable(ptrChunkEnable)) {
        std::cout << "SpinnakerCamera configureChunkData(): Not available"
                  << std::endl;
        throw std::invalid_argument("Unable to write node: ChunkEnable");
      }
      ptrChunkEnable->SetValue(true);
    }
  } catch (Spinnaker::Exception &e) {
    std::cout << e.what() << std::endl;
  }
}

int setCameraSetting(const string &node, const string &value) {
  INodeMap &nodeMap = camera->GetNodeMap();

  // Retrieve enumeration node from nodemap
  CEnumerationPtr ptr = nodeMap.GetNode(node.c_str());
  if (!IsAvailable(ptr)) {
    return -1;
  }
  if (!IsWritable(ptr)) {
    return -1;
  }
  // Retrieve entry node from enumeration node
  CEnumEntryPtr ptrValue = ptr->GetEntryByName(value.c_str());
  if (!IsAvailable(ptrValue)) {
    return -1;
  }
  if (!IsReadable(ptrValue)) {
    return -1;
  }
  // retrieve value from entry node
  const int64_t valueToSet = ptrValue->GetValue();

  // Set value from entry node as new value of enumeration node
  ptr->SetIntValue(valueToSet);

  return 0;
}

int setCameraSetting(const string &node, int val) {
  INodeMap &nodeMap = camera->GetNodeMap();

  CIntegerPtr ptr = nodeMap.GetNode(node.c_str());
  if (!IsAvailable(ptr)) {
    return -1;
  }
  if (!IsWritable(ptr)) {
    return -1;
  }
  ptr->SetValue(val);
  return 0;
}
int setCameraSetting(const string &node, float val) {
  INodeMap &nodeMap = camera->GetNodeMap();
  CFloatPtr ptr = nodeMap.GetNode(node.c_str());
  if (!IsAvailable(ptr)) {
    return -1;
  }
  if (!IsWritable(ptr)) {
    return -1;
  }
  ptr->SetValue(val);

  return 0;
}
int setCameraSetting(const string &node, bool val) {
  INodeMap &nodeMap = camera->GetNodeMap();

  CBooleanPtr ptr = nodeMap.GetNode(node.c_str());
  if (!IsAvailable(ptr)) {
    return -1;
  }
  if (!IsWritable(ptr)) {
    return -1;
  }
  ptr->SetValue(val);

  return 0;
}

int setPixFmt(const string &fmt) {
  return setCameraSetting("PixelFormat", string(fmt));
}

void resetUserSet() {
  std::this_thread::sleep_for(std::chrono::seconds(1));
  camera->UserSetSelector.SetValue(
      Spinnaker::UserSetSelectorEnums::UserSetSelector_Default);

  camera->UserSetLoad.Execute();
  std::this_thread::sleep_for(std::chrono::seconds(1));
}

int setROI(int offsetX, int offsetY, int imwidth, int imheight) {
  // remove offsets, to allow large(r) image areas
  if (setCameraSetting("OffsetX", 0) == -1) {
    return -1;
  }
  if (setCameraSetting("OffsetY", 0) == -1) {
    return -1;
  }
  // now set specified image areas
  if (setCameraSetting("Height", imheight) == -1) {
    return -1;
  }
  if (setCameraSetting("Width", imwidth) == -1) {
    return -1;
  }
  // now set offsets, may fail, if too large for above image area
  if (setCameraSetting("OffsetX", offsetX) == -1) {
    return -1;
  }
  if (setCameraSetting("OffsetY", offsetY) == -1) {
    return -1;
  }

  return 0;
}
int setExposureTime(float exposure_time) {
  return setCameraSetting("ExposureTime", exposure_time);
}

int setExposureAuto(const string &mode) {
  return setCameraSetting("ExposureAuto", mode);
}

int setGainAutoDisable() { return setCameraSetting("GainAuto", string("Off")); }

int setSharpeningDisable() {
  return setCameraSetting("SharpeningEnable", false);
}

int setWhiteBalanceAuto() {
  return setCameraSetting("BalanceWhiteAuto", string("Continuous"));
}

int main(int argc, char *argv[]) {
  int width = 2048;
  int height = 1536;
  int offsetX = 0;
  int offsetY = 0;
  float exposure = 100;

  Spinnaker::SystemPtr system_ptr = Spinnaker::System::GetInstance();

  // Retrieve list of cameras from the system_ptr
  Spinnaker::CameraList camList = system_ptr->GetCameras();

  camera = camList.GetBySerial("20109543");
  camList.Clear();

  // Initialize camera
  camera->Init();

  // ensure we start from a clean slate
  // This crashes with "Writing to Register error" usually. Unclear why userset
  // modification is so brittle
  resetUserSet();

  // Set acquisition mode to continuous
  if (setCameraSetting("AcquisitionMode", string("Continuous")) == -1) {
    throw std::runtime_error("Could not set AcquisitionMode");
  }

  /// <------------ Problem here ----------------->
  configureChunkData(camera->GetNodeMap());

  // Enable this here to trigger the problem
  setCameraSetting("AcquisitionFrameRateEnabled", true);
  setCameraSetting("AcquisitionFrameRateEnable", true);
  setCameraSetting("AcquisitionFrameRateAuto", std::string("Off"));

  // Disable auto exposure to be safe. otherwise we cannot manually set
  // exposure time.
  setCameraSetting("ExposureAuto", string("Off"));

  setExposureTime(exposure);

  // important, otherwise we don't get frames at all
  if (setPixFmt("BayerGR8") == -1) {
    throw std::runtime_error("Could not set pixel format");
  }

  // When the above is enabled, trying this here does not fix the problem.
  const float fps = 25;
  int res = setCameraSetting("AcquisitionFrameRate", fps);
  if (res != 0) {
    throw std::runtime_error("Could not set frame rate");
  }

  setROI(offsetX, offsetY, width, height);

  camera->BeginAcquisition();

  long cycle_count = 0;

  int successes = 0;

  // wait a bit for camera to start streaming
  std::this_thread::sleep_for(std::chrono::seconds(4));

  // try 100 times to get a frame with some retry logic. we wait at the end to
  // retrieve only every 30ms
  const int n_trials = 250;
  for (int i = 0; i < n_trials; ++i) {
    auto tic = std::chrono::system_clock::now();
    Spinnaker::ImagePtr currentFrame = nullptr;
    unsigned int reps = 0;

    uint64_t timeout64 = Spinnaker::EVENT_TIMEOUT_NONE;

    do {
      try {
        currentFrame = camera->GetNextImage(timeout64);
        if (currentFrame->IsIncomplete()) {
          std::cerr << "Frame on secondary camera incomplete. Wait small "
                       "amount, if first rep."
                    << std::endl;
          currentFrame->Release();
          currentFrame = nullptr;
          timeout64 = 2;
        } else if (currentFrame->GetImageStatus() !=
                   Spinnaker::IMAGE_NO_ERROR) {
          std::cerr << "There was an error on GetNextImage(): "
                    << Spinnaker::Image::GetImageStatusDescription(
                           currentFrame->GetImageStatus())
                    << std::endl;
          currentFrame->Release();
          currentFrame = nullptr;
          break; // no repetition if error occured
        }
      } catch (const Spinnaker::Exception &e) {
        std::cerr << "Spinnaker camera GetNextImage() failed on secondary "
                     "camera with "
                  << e.what() << ". Retrying." << std::endl;
        currentFrame = nullptr;
        break; // no repetition if image hasn't started or problem with cam
      }
      // Repeat 2x if incomplete frame on secondary camera.
    } while (++reps < 2 && !currentFrame);

    if (!currentFrame) {
      std::cerr << "Dropped frame." << std::endl;
    } else {
      ++successes;
    }

    ++cycle_count;
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          tic - std::chrono::system_clock::now())
                          .count();
    if (elapsed_ms < 30) {
      auto sleep_duration = std::chrono::milliseconds(30 - elapsed_ms);
      std::cout << "Sleeping for " << sleep_duration.count() << std::endl;
      std::this_thread::sleep_for(sleep_duration);
    }
  }

  std::cout << "Got " << successes << " frames in " << n_trials << " attempts."
            << std::endl;

  // End acquisition
  camera->EndAcquisition();
  // Deinitialize camera
  camera->DeInit();
  // when system is released in same scope, camera ptr must be cleaned up before
  camera = nullptr;
  camList.Clear();
  // Release system_ptr
  system_ptr->ReleaseInstance();
  system_ptr = nullptr;

  return 0;
}
