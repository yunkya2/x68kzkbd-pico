# Raspberry Pi Pico による X68000Z Keyboard LED 制御デモ

## 概要

[瑞起 X68000Z EARLY ACCESS KIT](https://www.zuiki.co.jp/x68000z/) に付属するキーボードのLEDを、 [Raspberry Pi Pico](https://www.raspberrypi.com/products/raspberry-pi-pico/) の USBホスト機能を用いて制御します。

## 必要なもの

* 瑞起 X68000Z EARLY ACCESS KIT 付属キーボード
  * HACKER'S EDITION付属や単体売り版キーボードでも動くと思いますが未確認です
* Raspberry Pi Pico (Pico Wでも可)
* USB OTG ケーブル (USB Type-Aメス - micro-B のケーブル)
* Pico への電源供給手段
  * Raspberry Pi Pico は通常、USB端子(micro-B)から電源を供給しますが、USBホスト機能を使用する場合は逆に Pico からUSBへの電源供給が必要になります。
  * Raspberry Pi Picoを2枚使って片方をデバッガにしているのであれば、デバッガ側のVSYSを実行側のVBUSに繋いでやることで、電源供給が可能です。

## ビルド方法

* PCにPico SDKをインストールして、PICO_SDK_PATH 環境変数がSDKの位置を指すようにしておきます。
* 本リポジトリをcloneして、make を実行します。
* build/ の下にELFファイルやUF2ファイルが出来るので、デバッガを起動するなどしてPicoに書き込みます。

## 実行

* 実行すると USB OTG ケーブルに繋いだX68000Zキーボードを認識して、LEDの制御デモが動き出します。
* キーボードのキーを押すと、
  ```
  mod:00 key:2c 00 00 00 00 00
  ```
  のように、押されているモディファイヤキーの情報とその時押されているキーコードを最大6つまで表示します。

## X68000Z キーボードのLED制御について

(筆者の独自解析によるため、内容の正確性は保証しません）

* X68000Z のキーボードは1つのコンフィグレーションに対して2つのインターフェースディスクリプタを持っています。
* インターフェース0は通常のUSBキーボードと同じですが、インターフェース1は複数のHIDレポートIDを持ち(1, 2, 5, 6, 10)、そのうちID 10のFeatureレポートに対してHIDクラスリクエストのSET_REPORTを実行することで、LEDの制御ができるようです。
* SET_REPORTでは必ず65バイト (ID 1バイト + データ本体64バイト)のデータを送り、その中の値でLEDの状態を設定します。
  * data[0] = 0x0a (10) .... レポートID
  * data[1] = 0xf8      .... (LED設定コマンド？)
  * data[7] = かな LEDの明るさ (0x00～0xffで値が大きいほど明るい)
  * data[8] = ローマ字 LED
  * data[9] = コード入力 LED
  * data[10] = CAPS LED
  * data[11] = INS LED
  * data[12] = (未使用)
  * data[13] = ひらがな LED
  * data[14] = 全角 LED
  * 他のデータの値は 0x00 にしておきます
* ソースコード hid_app.c の led_report_task(), led_report_set() が該当の処理となります。

## ライセンス

本プログラムは pico-examplesのサンプル [host_cdc_msc_hid](https://github.com/raspberrypi/pico-examples/tree/master/usb/host/host_cdc_msc_hid) を元にしています。
オリジナルのライセンス条件と同様にMITライセンスとします。
