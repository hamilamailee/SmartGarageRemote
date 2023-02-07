//
//  ViewController.swift
//  SmartGarageRemote
//
//  Created by Saee Saadat on 2/3/23.
//

import UIKit
import CocoaMQTT

class ViewController: UIViewController {

    
    @IBOutlet weak var mainStackView: UIStackView!
    @IBOutlet weak var topStackView: UIStackView!
    @IBOutlet weak var bottomStackView: UIStackView!
    
    @IBOutlet var buttons: [UIButton]!
    @IBOutlet weak var wifiSettingsButton: UIButton!
    
    private var learnButton: UIButton?
    private var isInLearnMode: Bool = false
    
    private var mqttHandler = MQTTHandler()
    
    override func viewDidLoad() {
        super.viewDidLoad()
        self.setupView()
        self.setupMqtt()
    }
    
    private func setupMqtt() {
        mqttHandler.mqttClient.delegate = self
    }
    
    private func setupView() {
        setupButtons()
    }
    
    private func setupButtons() {
        for b in buttons {
            b.layer.cornerRadius = b.frame.width / 2
            b.layer.borderWidth = 1
            b.layer.borderColor = UIColor.black.cgColor
            
            if b.restorationIdentifier == "learn" {
                self.learnButton = b
                b.addTarget(self, action: #selector(self.learn(sender:)), for: .touchUpInside)
            } else {
                b.addTarget(self, action: #selector(self.buttonClicked(sender:)), for: .touchUpInside)
                b.addTarget(self, action: #selector(self.transformOnClick(sender:)), for: .touchDown)
                b.addTarget(self, action: #selector(self.transformOnRelease(sender:)), for: .touchUpOutside)
            }
        }
    }

    @objc
    private func buttonClicked(sender: UIButton) {
        transformOnRelease(sender: sender)
        guard let signalId = Int(sender.restorationIdentifier ?? "") else {
            return
        }
        if isInLearnMode, let b = self.learnButton {
            learn(sender: b)
            sendLearnSignal(signalId: signalId)
        } else {
            sendTransmitSignal(signalId: signalId)
        }
    }
    
    @objc
    private func transformOnClick(sender: UIButton) {
        sender.transform = CGAffineTransform(scaleX: 0.8, y: 0.8)
    }
    
    @objc
    private func transformOnRelease(sender: UIButton) {
        sender.transform = .identity
    }

    @objc
    private func learn(sender: UIButton) {
        self.isInLearnMode = !self.isInLearnMode
        if isInLearnMode {
            sender.backgroundColor = UIColor(named: "Yelo darker")
            sender.transform = CGAffineTransform(scaleX: 0.8, y: 0.8)
        } else {
            sender.backgroundColor = UIColor(named: "Yelo")
            sender.transform = .identity
        }
    }
    
    private func sendLearnSignal(signalId: Int) {
        do {
            try mqttHandler.sendSignal(.learn(id: signalId))
        } catch (let e) {
            print("error thrown damn: \(e)")
        }
    }
    
    private func sendTransmitSignal(signalId: Int) {
        do {
            try mqttHandler.sendSignal(.transmit(id: signalId))
        } catch (let e) {
            print("error thrown damn: \(e)")
        }
    }
    
    @IBAction func wifiSettingsClicked(_ sender: Any) {
        let vc = WifiViewController(nibName: "WifiViewController", bundle: nil)
        self.present(vc, animated: true)
    }
    
    private func handleNewMessage(_ message: MqttCustomMessage) {
        if message.mode == "error" {
            showToast(message: message.message, isError: true)
        } else {
            showToast(message: message.message)
        }
    }
    
}

extension ViewController: CocoaMQTT5Delegate {
    func mqtt5(_ mqtt5: CocoaMQTT5, didConnectAck ack: CocoaMQTTCONNACKReasonCode, connAckData: MqttDecodeConnAck?) {
        print("Connect Ack! \(String(describing: connAckData))")
        mqttHandler.mqttClient.subscribe("/messages")
    }
    
    func mqtt5(_ mqtt5: CocoaMQTT5, didPublishMessage message: CocoaMQTT5Message, id: UInt16) {
        print("did publish message with id \(id) -> \(message)")
    }
    
    func mqtt5(_ mqtt5: CocoaMQTT5, didPublishAck id: UInt16, pubAckData: MqttDecodePubAck?) {
        print("did publish ack with id \(id) -> \(String(describing: pubAckData))")
    }
    
    func mqtt5(_ mqtt5: CocoaMQTT5, didPublishRec id: UInt16, pubRecData: MqttDecodePubRec?) {
        print("did publish rec with id \(id) -> \(String(describing: pubRecData))")
    }
    
    func mqtt5(_ mqtt5: CocoaMQTT5, didReceiveMessage message: CocoaMQTT5Message, id: UInt16, publishData: MqttDecodePublish?) {
        print("did receive message with id \(id) -> \(String(describing: message))")

        if let payload = String(bytes: message.payload, encoding: .utf8) {
            print("payload: \(payload)")
            let data = payload.data(using: .utf8, allowLossyConversion: false)!
            let decoder = JSONDecoder()
            if let message = try? decoder.decode(MqttCustomMessage.self, from: data) {
                handleNewMessage(message)
            }
            
        }
    }
    
    func mqtt5(_ mqtt5: CocoaMQTT5, didSubscribeTopics success: NSDictionary, failed: [String], subAckData: MqttDecodeSubAck?) {
        print("did subscribe to \(success)")
    }
    
    func mqtt5(_ mqtt5: CocoaMQTT5, didUnsubscribeTopics topics: [String], UnsubAckData: MqttDecodeUnsubAck?) {
        print("did unsubscribe")
    }
    
    func mqtt5(_ mqtt5: CocoaMQTT5, didReceiveDisconnectReasonCode reasonCode: CocoaMQTTDISCONNECTReasonCode) {
        print("did receive disconnect reason code \(reasonCode)")
    }
    
    func mqtt5(_ mqtt5: CocoaMQTT5, didReceiveAuthReasonCode reasonCode: CocoaMQTTAUTHReasonCode) {
        print("did receive auth reason code \(reasonCode)")
    }
    
    func mqtt5DidPing(_ mqtt5: CocoaMQTT5) {
    }
    
    func mqtt5DidReceivePong(_ mqtt5: CocoaMQTT5) {
    }
    
    func mqtt5DidDisconnect(_ mqtt5: CocoaMQTT5, withError err: Error?) {
        print("disconnected! \(String(describing: err))")
    }
    
    
}
