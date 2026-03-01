# M5Core2-NES

M5Stack Core2 向けの NES (ファミコン) エミュレータです。nofrendo エンジンをベースにしています。

## 準備

1. SDカードのルートに `nes` フォルダを作成
2. `.nes` ROMファイルを `nes` フォルダに入れる
3. SDカードをM5Core2に挿入して起動

## ROM選択

複数のROMがある場合、起動時に選択画面が表示されます。

- リスト内のタップ ... カーソル移動
- 同じ項目をもう一度タップ ... そのROMで起動
- 画面下部 `[UP]` ... 上にスクロール
- 画面下部 `[SELECT]` ... 選択中のROMで起動
- 画面下部 `[DOWN]` ... 下にスクロール

ROMが1つだけの場合は自動で読み込まれます。

## ゲーム中の操作

画面をタッチして操作します。画面は3x3の9ゾーンに分かれています。

```
+----------+----------+----------+
|          |          |          |
|    UP    |   DOWN   |    A     |
|          |          |          |
+----------+----------+----------+
|          |          |          |
|   LEFT   |  RIGHT   |    B     |
|          |          |          |
+----------+----------+----------+
|          |          |          |
|  SELECT  |  START   |  SOUND   |
|          |          |          |
+----------+----------+----------+
  x<100     100-200     x>=200
```

| ゾーン | 位置 | NESボタン |
|--------|------|-----------|
| 左上 | x<100, y<90 | UP |
| 中上 | 100<=x<200, y<90 | DOWN |
| 右上 | x>=200, y<90 | A |
| 左中 | x<100, 90<=y<180 | LEFT |
| 中央 | 100<=x<200, 90<=y<180 | RIGHT |
| 右中 | x>=200, 90<=y<180 | B |
| 左下 | x<100, y>=180 | SELECT |
| 中下 | 100<=x<200, y>=180 | START |
| 右下 | x>=200, y>=180 | SOUND ON/OFF |

2点同時タッチに対応しています。

## サウンド

- サウンドはデフォルトで **OFF** です
- 画面右下をタッチするとON/OFF切替できます
- M5Core2の内蔵スピーカーの制約上、音質には限界があります

## Arduino IDE でのビルド

### 必要なライブラリ

- M5Core2
- M5Stack-SD-Updater

### ボード設定

- ボード: `M5Stack Core2`
- パーティション: `Huge APP`

### Arduino CLI でのビルド

```
arduino-cli compile --fqbn m5stack:esp32:m5stack_core2 \
  --build-property "build.partitions=huge_app" \
  --build-property "upload.maximum_size=3145728" \
  --build-property "compiler.c.extra_flags=-DCONFIG_SOUND_ENA=1 -Isrc/nofrendo -Isrc/nofrendo/nes -Isrc/nofrendo/sndhrdw -Isrc/nofrendo/mappers -Isrc/nofrendo/cpu -Isrc/nofrendo/libsnss -Isrc/nofrendo-esp32" \
  --build-property "compiler.cpp.extra_flags=-DCONFIG_SOUND_ENA=1 -Isrc/nofrendo -Isrc/nofrendo/nes -Isrc/nofrendo/sndhrdw -Isrc/nofrendo/mappers -Isrc/nofrendo/cpu -Isrc/nofrendo/libsnss -Isrc/nofrendo-esp32" \
  .

arduino-cli upload --fqbn m5stack:esp32:m5stack_core2 --port COM11 .
```

サウンドを無効にしてビルドする場合は `-DCONFIG_SOUND_ENA=1` を除去してください。

## クレジット

- [nofrendo](http://www.baisoku.org/) - NES emulator engine by Matthew Conte
- [m5core2_esp-idf_nesemu](https://github.com/HidetoKimura/m5core2_esp-idf_nesemu) - Original ESP-IDF port
