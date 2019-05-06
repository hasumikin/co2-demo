# CO2プレゼン方法

- `make monitor >> log/log.txt` でモニタ出力をファイルに書き出す。teeだとブロックされてうまくいかない
- 別ウィンドウで `cd log`
- `ruby tail-f.rb` で整形&時間つきデータに
- `cd server && ruby app.rb` で表示サーバ起動
- `locahost:4567/chart.html` で表示
