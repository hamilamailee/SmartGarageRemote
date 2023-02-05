//
//  ToastExtension.swift
//  SmartGarageRemote
//
//  Created by Saee Saadat on 2/4/23.
//

import UIKit

extension UIViewController {

    func showToast(message : String, font: UIFont = .systemFont(ofSize: UIFont.systemFontSize * 1.5), isError: Bool = false) {

        let toastLabel = UILabel(frame: CGRect(x: self.view.frame.size.width/2 - 100, y: self.view.frame.size.height-100, width: 200, height: 50))
        toastLabel.backgroundColor = isError ? .red.withAlphaComponent(0.8) : .black.withAlphaComponent(0.6)
        toastLabel.textColor = UIColor.white
        toastLabel.font = font
        toastLabel.textAlignment = .center;
        toastLabel.text = message
        toastLabel.alpha = 1.0
        toastLabel.layer.cornerRadius = 10;
        toastLabel.clipsToBounds  =  true
        self.view.addSubview(toastLabel)
        UIView.animate(withDuration: 4.0, delay: 0.1, options: .curveEaseOut, animations: {
             toastLabel.alpha = 0.0
        }, completion: {(isCompleted) in
            toastLabel.removeFromSuperview()
        })
    }
    
}
