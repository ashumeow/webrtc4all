using System;
using System.Collections.Generic;
using System.Text;
using webrtc4ieLib;

namespace testCSharp
{
    class Program
    {
        static void Main(string[] args)
        {
            // actions, for setLocalDescription/setRemoteDescription
            const ushort SDP_OFFER = 0x100;
            const ushort SDP_PRANSWER = 0x200;
            const ushort SDP_ANSWER = 0x300;

            // PeerConnection state
            const ushort NEW = 0;     // initial state
            const ushort OPENING = 1; // local or remote desc set
            const ushort ACTIVE = 2;  // local and remote desc set
            const ushort CLOSED = 3;  // ended state

            // ICE state
            const ushort ICE_GATHERING = 0x100;
           const ushort ICE_WAITING = 0x200;
           const ushort ICE_CHECKING = 0x300;
           const ushort ICE_CONNECTED = 0x400;
           const ushort ICE_COMPLETED = 0x500;
           const ushort ICE_FAILED = 0x600;
           const ushort ICE_CLOSED = 0x700;


           /*NetTransport transport = new NetTransport();
           transport.OnEvent += (type, data) =>
               {
                   Console.WriteLine("transport.OnEvent({0}, {1})", type, data);
               };

           transport.SetDomain("sip2sip.info");
           Console.WriteLine("Default DstAddr= {0} Dst Port = {1}", transport.defaultDestAddr, transport.defaultDestPort);

           transport.Start(0);
           transport.SendTo("salut", "192.168.0.10", 5060);
           transport.Stop();*/




            PeerConnection peerConnection = new PeerConnection();

            String ro = peerConnection.createAnswer(true, true);
            SessionDescription sdpOffer = new SessionDescription();
            sdpOffer.Init(ro);

            Console.WriteLine("Offer before ICE = {0}", sdpOffer.toSdp());

            peerConnection.IceCallback += (media, candidate, moreToFollow) => 
            {
                sdpOffer.addCandidate(media, candidate);
                if (!moreToFollow)
                {
                    String sdp = sdpOffer.toSdp();
                    Console.WriteLine("Offer after ICE = {0}", sdp);
                    peerConnection.setRemoteDescription(SDP_ANSWER, sdp);
                }
            };



            peerConnection.startIce(0, 0);

            Console.ReadLine();

            peerConnection.close();
        }

        
    }
}
