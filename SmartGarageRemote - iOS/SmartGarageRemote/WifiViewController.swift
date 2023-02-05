//
//  WifiViewController.swift
//  SmartGarageRemote
//
//  Created by Saee Saadat on 2/4/23.
//

import UIKit
import SystemConfiguration.CaptiveNetwork
import CoreLocation
import NetworkExtension
import Combine

class WifiViewController: UIViewController, CLLocationManagerDelegate, UITextFieldDelegate {

    @IBOutlet weak var connectDeviceCard: UIView!
    @IBOutlet weak var connectDeviceLabel: UILabel!
    @IBOutlet weak var connectDeviceDescLabel: UILabel!
    @IBOutlet weak var connectDeviceButton: UIButton!
    
    @IBOutlet weak var connectWifiCard: UIView!
    @IBOutlet weak var wifiNameTextfield: UITextField!
    @IBOutlet weak var wifiPasswordTextField: UITextField!
    @IBOutlet weak var connectWifiButton: UIButton!
    
    var locationManager = CLLocationManager()
    var observers: [AnyCancellable] = []
    var currentNetworkInfos: Array<NetworkInfo>? {
        get {
            return SSID.fetchNetworkInfo()
        }
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        setupView()
//        setupWifiName()
        hideKeyboardWhenTappedAround()
    }
    
    private func setupView() {
        for v in [wifiNameTextfield, wifiPasswordTextField, connectDeviceButton, connectWifiButton] {
            v?.layer.cornerRadius = 10
            v?.clipsToBounds = true
            v?.layer.borderWidth = 1
            v?.layer.borderColor = UIColor(named: "b")?.cgColor
            if let v = v as? UITextField {
                v.delegate = self
            }
        }
        connectDeviceDescLabel.text = (connectDeviceDescLabel.text ?? "Connect to ") + "test2"
    }
    
//    private func setupWifiName() {
//        let status = CLLocationManager.authorizationStatus()
//        if status == .authorizedWhenInUse {
//            updateWiFi()
//        } else {
//            locationManager.delegate = self
//            locationManager.requestWhenInUseAuthorization()
//        }
//    }
//
    
    private func updateWiFi() {
        print("SSID: \(currentNetworkInfos?.first?.ssid ?? "")")
        
        if let ssid = currentNetworkInfos?.first?.ssid {
            wifiNameTextfield.text = "SSID: \(ssid)"
        }
        
        if let bssid = currentNetworkInfos?.first?.bssid {
            wifiNameTextfield.text = "BSSID: \(bssid)"
        }
        
    }
    
    @IBAction private func connectDevicePressed(_ sender: UIButton) {
        
        if let url = URL(string:UIApplication.openSettingsURLString) {
            if UIApplication.shared.canOpenURL(url) {
                UIApplication.shared.open(url, options: [:], completionHandler: nil)
            }
        }
        
    }
    
    @IBAction private func connectWifiPressed(_ sender: UIButton) {
        connectWifiButton.isEnabled = false
        guard let ssid = wifiNameTextfield.text, let pass = wifiPasswordTextField.text, !ssid.isEmpty else { return }
        postRequest(ssid: ssid, pass: pass, complitionHandler: { [weak self] in
            DispatchQueue.main.async {
                self?.dismiss(animated: true)
            }
        })
    }
    
    private func postRequest(ssid: String, pass: String, complitionHandler: @escaping (() -> Void)) {
        

        let url = URL(string: "192.168.1.1/connect")!
        let session = URLSession.shared
        var request = URLRequest(url: url)
        request.httpMethod = "POST"
        request.httpBody = "ssid=\(ssid)&pass=\(pass)".data(using: .utf8)
        
        // create dataTask using the session object to send data to the server
        let task = session.dataTask(with: request) { data, response, error in
            
            if let error = error {
                print("Post Request Error: \(error.localizedDescription)")
                return
            }
            
            // ensure there is valid response code returned from this HTTP response
            guard let httpResponse = response as? HTTPURLResponse,
                  (200...299).contains(httpResponse.statusCode)
            else {
                print("Invalid Response received from the server")
                return
            }
            
            // ensure there is data returned
            guard let responseData = data else {
                print("nil Data received from the server")
                return
            }
            
            defer {
                complitionHandler()
            }
            
            do {
                // create json object from data or use JSONDecoder to convert to Model stuct
                if let jsonResponse = try JSONSerialization.jsonObject(with: responseData, options: .mutableContainers) as? [String: Any] {
                    print(jsonResponse)
                    
                    // handle json response
                } else {
                    print("data maybe corrupted or in wrong format")
                    throw URLError(.badServerResponse)
                }
            } catch let error {
                print(error.localizedDescription)
            }
        }
        // perform the task
        task.resume()
    }
    
    func textFieldShouldReturn(_ textField: UITextField) -> Bool {
        self.view.endEditing(true)
        return false
    }

}


public class SSID {
    class func fetchNetworkInfo() -> [NetworkInfo]? {
        if let interfaces: NSArray = CNCopySupportedInterfaces() {
            var networkInfos = [NetworkInfo]()
            for interface in interfaces {
                let interfaceName = interface as! String
                var networkInfo = NetworkInfo(interface: interfaceName,
                                              success: false,
                                              ssid: nil,
                                              bssid: nil)
                if let dict = CNCopyCurrentNetworkInfo(interfaceName as CFString) as NSDictionary? {
                    networkInfo.success = true
                    networkInfo.ssid = dict[kCNNetworkInfoKeySSID as String] as? String
                    networkInfo.bssid = dict[kCNNetworkInfoKeyBSSID as String] as? String
                }
                networkInfos.append(networkInfo)
            }
            return networkInfos
        }
        return nil
    }
}

struct NetworkInfo {
    var interface: String
    var success: Bool = false
    var ssid: String?
    var bssid: String?
}

extension UIViewController {
    func hideKeyboardWhenTappedAround() {
        let tap = UITapGestureRecognizer(target: self, action: #selector(UIViewController.dismissKeyboard))
        tap.cancelsTouchesInView = false
        view.addGestureRecognizer(tap)
    }
    
    @objc func dismissKeyboard() {
        view.endEditing(true)
    }
}
