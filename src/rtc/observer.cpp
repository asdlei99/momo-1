#include <iostream>
#include "rtc_base/logging.h"

#include "observer.h"

void PeerConnectionObserver::OnStandardizedIceConnectionChange(
    webrtc::PeerConnectionInterface::IceConnectionState new_state) {
  RTC_LOG(LS_ERROR) << __FUNCTION__ << " :" << new_state;
  if (_recieve_track_handler != nullptr &&
      new_state == webrtc::PeerConnectionInterface::
        IceConnectionState::kIceConnectionDisconnected) {
    for (webrtc::VideoTrackInterface* video_track : _video_tracks) {
      _recieve_track_handler->RemoveTrack(video_track);
    }
    _video_tracks.clear();
  }
  if (_sender != nullptr) {
    _sender->onIceConnectionStateChange(new_state);
  }
}

void PeerConnectionObserver::OnIceCandidate(
    const webrtc::IceCandidateInterface* candidate) {
  std::string sdp;
  if (candidate->ToString(&sdp)) {
    if (_sender != nullptr) {
      _sender->onIceCandidate(candidate->sdp_mid(),
                              candidate->sdp_mline_index(), sdp);
    }
  } else {
    RTC_LOG(LS_ERROR) << "Failed to serialize candidate";
  }
}

void PeerConnectionObserver::OnTrack(
    rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver) {
  if (_recieve_track_handler == nullptr) return;
  webrtc::MediaStreamTrackInterface* track = transceiver->receiver()->track().release();
  if (track->kind() == webrtc::MediaStreamTrackInterface::kVideoKind) {
    webrtc::VideoTrackInterface* video_track = static_cast<webrtc::VideoTrackInterface*>(track);
    _video_tracks.push_back(video_track);
    _recieve_track_handler->AddTrack(video_track);
  }
}

void PeerConnectionObserver::OnRemoveTrack(
    rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) {
  if (_recieve_track_handler == nullptr) return;
  webrtc::MediaStreamTrackInterface* track = receiver->track().release();
  if (track->kind() == webrtc::MediaStreamTrackInterface::kVideoKind) {
    webrtc::VideoTrackInterface* video_track = static_cast<webrtc::VideoTrackInterface*>(track);
    _video_tracks.erase(
      std::remove_if(
          _video_tracks.begin(), _video_tracks.end(),
          [video_track](const webrtc::VideoTrackInterface* track) {
            return track == video_track;
          }),
      _video_tracks.end());
    _recieve_track_handler->RemoveTrack(video_track);
  }
}

void CreateSessionDescriptionObserver::OnSuccess(
    webrtc::SessionDescriptionInterface* desc) {
  std::string sdp;
  desc->ToString(&sdp);
  RTC_LOG(LS_INFO) << "Created session description : " << sdp;
  _connection->SetLocalDescription(
      SetSessionDescriptionObserver::Create(desc->GetType(), _sender), desc);
  if (_sender != nullptr) {
    _sender->onCreateDescription(desc->GetType(), sdp);
  }
}

void CreateSessionDescriptionObserver::OnFailure(webrtc::RTCError error) {
  RTC_LOG(LS_ERROR) << "Failed to create session description : "
                    << ToString(error.type()) << ": " << error.message();
}

void SetSessionDescriptionObserver::OnSuccess() {
  RTC_LOG(LS_INFO) << "Set local description success!";
  if (_sender != nullptr) {
    _sender->onSetDescription(_type);
  }
}

void SetSessionDescriptionObserver::OnFailure(webrtc::RTCError error) {
  RTC_LOG(LS_ERROR) << "Failed to set local description : "
                    << ToString(error.type()) << ": " << error.message();
}