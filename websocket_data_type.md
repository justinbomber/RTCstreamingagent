## websocket request type

## 1. 請求類型 (json):
### request type (json):
| 名稱             | 類型            | 描述                             | 範例                                    |
|------------------|-----------------|---------------------------------|---------------------------------------|
| token            | string          | 訪問tab的token，唯一識別碼                     | `"1697439121-123e4567e89b12d3a456426655440000"` |
|username          |string|          |使用者的uuid|`123e4567e89b12d3a456426655440000`|
| ai_type          | array(string)   | 包含查詢類型的數組，若沒有則維持空           | `"["cat", "dog"]"`                       |
| partition_device | string          | 分區設備的來源, ***位置_x 位置_y / 來源*** | `"103365/6ba7b810-9dad-11d1-80b4-00c04fd430c8"` |
| query_type       | int             | 查詢的類型  ***0:歷史串流 1:即時串流***  | `1`                                   |
| starttime        | int             | 影片開始時間，如果是即時影像則為0   | `116189209`                             |
| endtime          | int             | 影片結束時間，如果是即時影像則為0   | `116890000`                             |
| path             | string          | 資料的路徑 (uuid)                 | `"6ba7b810-9dad-11d1-80b4-00c04fd430c8"` |
| resolution       | string          | 資料的解析度, ***"480" 或 "1080"***   | `"1080"` |
| activate         | bool            | 此影片是否還在播放                |      `true`    |

### Answer type 1 (json) 影片:
| 名稱          | 類型      | 描述               | 範例                                      |
|--------------|----------|------------------|-----------------------------------------|
| token        |string|訪問tab的token，唯一識別碼  |`"1697439121-123e4567e89b12d3a456426655440000"`|
| url          | string   |sdp |`"http://10.1.1.128:8088/ramdisk/catchoutput/justin68/Cam001/pathwewantano.m3u8"`|
| path         | string   | 資料的路徑 (uuid)           | `"6ba7b810-9dad-11d1-80b4-00c04fd430c8"`                                  |

### Answer type 2 (json) AI標籤:
| 名稱         | 類型              | 描述                       | 範例                                          |
|--------------|-----------------|------------------------|-------------------------------------------|
|token|string|訪問tab的token，唯一識別碼  |`"1697439121-123e4567e89b12d3a456426655440000"`|
| ai_type      | object          | AI查詢的結果及相對應的結構，   | `[ {"dog": [11679209,11679200] } , {"cat": [11689000]}]`   |

### AI_type 對照表
| 中文名   | 對應參數  |
|---------|--------|
| 人      | human   |
| 鳥      | bird    |
| 貓      | cat     |
| 狗      | dog     |
| 交通燈  | light   |
| 消防栓  | hydrant |
| 車      | car     |
| 單車    | bike    |
| 摩托車  | motor   |
| 飛機    | plane   |
| 巴士    | bus     |
| 火車    | train   |
| 卡車    | truck   |
| 船      | boat    |

<!-- ## 2. 執行順序

1. user A 透過 `ws://server_ip:port/userA` 連線至websocket server。
2. user A 等待server返回 **token** 。
3. user A 斷開以用戶名建立的連線，即 `ws://server_ip:port/userA`。
4. user A 以 **token** 建立 URL為 `ws://server_ip:port/token` 的連線。
5. user A 依照第一部分(request type)的結構發送及接受請求。 -->


