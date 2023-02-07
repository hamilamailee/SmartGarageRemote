//
//  MQTTHandler.swift
//  SmartGarageRemote
//
//  Created by Saee Saadat on 2/4/23.
//

import Foundation
import CocoaMQTT

class MQTTHandler {
    public var mqttClient = CocoaMQTT5(clientID: "SmartGarageRemoteiOS - \(ProcessInfo().processIdentifier)", host: "addd6ec8.us-east-1.emqx.cloud", port: 15345)
    
    init() {
        mqttClient.username = "iosClient"
        mqttClient.password = "ios"
        mqttClient.allowUntrustCACertificate = true
        mqttClient.autoReconnect = true
        let conResult = mqttClient.connect()
        print("connection result: \(conResult)")
        
        mqttClient.subscribe("/garage", qos: .qos2)
    }
    
    public func sendData(data: Dictionary<String, Any>) throws {
        let publishProperties = MqttPublishProperties()
        publishProperties.contentType = "JSON"
        if let message = try String(data: JSONSerialization.data(withJSONObject: data), encoding: .ascii) {
            mqttClient.publish("/garage", withString: message, properties: publishProperties)
        }
    }
    
    public func sendMessage(_ msg: MqttCustomMessage) throws {
        let publishProperties = MqttPublishProperties()
        publishProperties.contentType = "JSON"
        if let message = try String(data: JSONEncoder().encode(msg), encoding: .ascii) {
            mqttClient.publish("/garage", withString: message, properties: publishProperties)
        }
    }
    
    public func sendSignal(_ signal: MqttSignal) throws {
        let publishProperties = MqttPublishProperties()
        publishProperties.contentType = "JSON"
        if let signalMessage = try signal.getJsonString() {
            mqttClient.publish("/garage", withString: signalMessage, properties: publishProperties)
        }
    }
}
