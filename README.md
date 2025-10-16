# CG Assignment 2

> 本次作業請同學實作以下：
> 1. 請同學下載 Moodle 提供的機械手臂.stl 檔案(也可自行繪製，須有四個旋轉軸、尺寸自訂)
> 2. 讀取上述之.stl 檔案並顯示於視窗 (60%)
> - 分別使用 QA、WS、ED、RF 鍵控制四個旋轉軸的旋轉角度 (30%)
>     1. Q、A 分別控制 link2 順、逆時針旋轉
>     2. W、S 分別控制 link3 順、逆時針旋轉
>     3. E、D 分別控制 link4 順、逆時針旋轉
>     4. R、F 分別控制 link5 順、逆時針旋轉
> 3. 程式架構是否清楚、簡潔 (10%)

## Compile Instructions
### Linux
```bash
make robotarm
```
### Windows Cross Compile (MinGW)
```bash
make robotarm.exe
```