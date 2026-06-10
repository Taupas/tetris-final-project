# 繳交項目一：GitHub Repository 連結

## Repository 連結

```
https://github.com/<你的GitHub使用者名稱>/tetris-final-project
```

**倉庫名稱：** `tetris-final-project`

---

## 一鍵推送（推薦）

在 `tetris` 資料夾雙擊執行：

```
push_github.bat
```

第一次會開啟瀏覽器登入 GitHub，完成後再執行一次即可推送。

---

## 手動命令

```bat
cd tetris
gh auth login
gh repo create tetris-final-project --public --source=. --remote=origin --push
git branch -M main
git push -u origin main
```

---

## 審閱者驗證

```bat
git clone https://github.com/<使用者名稱>/tetris-final-project.git
cd tetris-final-project
run.bat
```

瀏覽器：http://127.0.0.1:5000/tetris
