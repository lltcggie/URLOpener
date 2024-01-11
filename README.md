# URLOpener

URLによって開くブラウザを変えるツール

## セットアップ方法
`URLOpener.exe` をデフォルトブラウザにする必要がある。  
https://kimama9.blog.fc2.com/blog-entry-996.html  
ここ参照。

`URLOpener.exe` と同じディレクトリに `config.yaml` を設置する必要がある。
`config.yaml` でURLごとにどのブラウザを開くか設定する。

## 設定

```
"ブラウザのパス":
  - "URLマッチルール"
```

例
```
"C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe":
  - "dr:.*youtube.com"
  - "d:docs.google.com"
  - "https://github.com/"
"C:\\Program Files\\Mozilla Firefox\\firefox.exe":
  - "r:.*"
```

### URLマッチルール
- `d:` URLのドメインが完全に一致しているか
- `dr:` URLのドメインが正規表現に該当するかどうか
- `r:` URLが正規表現に該当するかどうか
- プレフィックスなし URLが前方一致するか
