Source code freely provided to you by [Doubango Telecom ®](http://doubango.org) under GPL v3 terms.



**webrtc4all** is a [WebRTC](http://en.wikipedia.org/wiki/WebRTC) extension for **Safari**, **Firefox**, **Opera** and **IE9+**. This extension is part of [sipML5](http://code.google.com/p/sipml5/) solution and implements Javascript PeerConnection API as defined in [draft-uberti-rtcweb-jsep-02](http://tools.ietf.org/html/draft-uberti-rtcweb-jsep-02). <br />
The goal is to allow developers to add  [WebRTC](http://en.wikipedia.org/wiki/WebRTC) features (audio/video streaming) in these browsers now and to be able to easily switch to the official implementation when it's added by Microsoft, Mozilla, Apple and Opera Software.

  * Live demo: [http://sipml5.org/call.htm](http://sipml5.org/call.htm?svn=6)
  * To download latest installer: http://code.google.com/p/webrtc4all/downloads/list
  * To checkout and build source code: http://code.google.com/p/webrtc4all/wiki/Building_Source
  * Javascript API: http://code.google.com/p/webrtc4all/source/browse/trunk/js/webrtc4all.js

## Compatibility ##
| | **Windows XP, Vista, 7 and 8** | **OS X** | **Unix** |
|:|:-------------------------------|:---------|:---------|
| Safari | Yes | coming soon | coming soon |
| Firefox | Yes | coming soon | coming soon |
| IE | Yes | coming soon | coming soon |
| Opera | Yes | coming soon | coming soon |

## Codecs ##
The media stack supports many codecs but some are disabled (e.g. G.729, AMR...). <br />
Enabled codecs:
  * Audio: G711, GSM, iLBC, Speex and G.722
  * Video: H.264, VP8, H.263, Theora and MP4V-ES

**Please note that this an alpha version and some functions could be missing or inconsistent.**

## Javacript API ##
Please take care of the prefix: **w4a** instead of **webkit**.
```
[Constructor (in DOMString configuration, in msIceCallback iceCb)]
interface w4aPeerConnection {
   w4aSessionDescription createOffer (msMediaHints hints);
   w4aSessionDescription createAnswer (DOMString offer, msMediaHints hints);

   const unsigned short SDP_OFFER = 0x100;
   const unsigned short SDP_PRANSWER = 0x200;
   const unsigned short SDP_ANSWER = 0x300;

   void setLocalDescription (unsigned short action, w4aSessionDescription desc);
   void setRemoteDescription (unsigned short action, w4aSessionDescription desc);
   
   readonly w4aSessionDescription localDescription;
   readonly w4aSessionDescription remoteDescription;

   const unsigned short NEW = 0;
   const unsigned short OPENING = 1;
   const unsigned short ACTIVE = 2;
   const unsigned short CLOSED = 3;

   readonly attribute unsigned short readyState;

   void startIce (optional w4aIceOptions options);
   void processIceMessage (w4aIceCandidate candidate);
   
   const unsigned short ICE_GATHERING = 0x100;
   const unsigned short ICE_WAITING = 0x200;
   const unsigned short ICE_CHECKING = 0x300;
   const unsigned short ICE_CONNECTED = 0x400;
   const unsigned short ICE_COMPLETED = 0x500;
   const unsigned short ICE_FAILED = 0x600;
   const unsigned short ICE_CLOSED = 0x700;
   readonly attribute unsigned short iceState;

   void addStream (w4aMediaStream stream, w4aMediaStreamHints hints);
   void removeStream (w4aMediaStream stream);
   readonly attribute w4aMediaStream[]  localStreams;
   readonly attribute w4aMediaStream[]  remoteStreams;
   void close ();
   [ rest of interface omitted ]
};
```

```
[Constructor (in DOMString sdp)]
interface w4aSessionDescription {
   void addCandidate(w4aIceCandidate candidate);
   DOMString toSdp();
};
```

```
[Constructor (in DOMString label, in DOMString candidateLine)]
interface w4aIceCandidate {
     readonly DOMString label;
     DOMString toSdp();
};
```

```
[Constructor (in Boolean has_audio, in Boolean has_video)]
interface msMediaHints {
    readonly Boolean has_audio;
    readonly Boolean has_video;
};
```

## Sample Code ##
```
var oSdpLocal, oSdpRemote;
var oPeerConnection;

// this is not part of the standard but is required
WebRtc4all_Init();

// creates the peerconnection
oPeerConnection = new w4aPeerConnection("STUN stun.l.google.com:19302",
        function (o_candidate, b_moreToFollow) {
            if (o_candidate) {
                oSdpLocal.addCandidate(o_candidate);
            }
            if (!b_moreToFollow) {
                // No more ICE candidates: 
                //Send the SDP message to the remote peer (e.g. as content of SIP INVITE request)
                SendSdp(oSdpLocal.toSdp());
            }
        }
);

// creates SDP offer, starts ICE and wait until ICE gathering finish (see above)
oSdpLocal = oPeerConnection.createOffer({ has_audio: true, has_video: true });
oPeerConnection.setLocalDescription(w4aPeerConnection.SDP_OFFER, oSdpLocal);
oPeerConnection.startIce({ use_candidates: "all" });

// some time later...we receive the SDP answer (DOMString) from the remote peer
onmessage = function(sSdpRemote){
 oSdpRemote = new w4aSessionDescription(sSdpRemote); // converts DOMString to "SessionDescription" object
 // set remote SDP and start media streaming
 oPeerConnection.setRemoteDescription(w4aPeerConnection.SDP_ANSWER, oSdpRemote);
};

// start media streaming
//oPeerConnection.startMedia();

// to stop audio/video streaming, shutdown ICE connections and clear resources
// oPeerConnection.close();
```

## Technical help ##
**Please don't send me technical questions by mail**. I'll certainly not respond to the mail as I receive more than a hundred messages per day. Sorry :)<br />
To get technical help, please subscribe to our <a href='http://groups.google.com/group/doubango'> developer's group</a> or use the <a href='http://code.google.com/p/webrtc4all/issues/list'>Issue tracker</a> to report bugs.

<br />
**© 2012 Doubango Telecom**
<br />
_Inspiring the future_