//
//  MqttMessage.swift
//  SmartGarageRemote
//
//  Created by Saee Saadat on 2/4/23.
//

import Foundation

class MqttCustomMessage: Decodable, Encodable {
    public var mode: String
    public var message: String
    
    init (mode: String, message: String) {
        self.mode = mode
        self.message = message
    }
    
    private enum CodingKyes: String, CodingKey {
        case mode = "mode"
        case message = "message"
    }
}

enum MqttSignal: Decodable, Encodable {
    case transmit(id: Int)
    case learn(id: Int)
    
    private var mode: String {
        switch self {
        case .transmit:
            return "transmit"
        case .learn:
            return "learn"
        }
    }
    
    private var id: Int {
        switch self {
        case .transmit(let id):
            return id
        case .learn(let id):
            return id
        }
    }
    
    public func getJsonString() throws -> String? {
        let dict: [String: Any] = ["mode": self.mode, "id": self.id]
        return String(data: try JSONSerialization.data(withJSONObject: dict), encoding: .ascii)
    }
}
