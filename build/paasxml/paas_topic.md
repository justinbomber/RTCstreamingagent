#PaaS #DDS #postgreSQL #Schema #Topic 

Date: 2023/09/25
### Type Structure & Topic ###

## Topic

| No | Topic Name      | Structure                | Note                              |
|----|-----------------|--------------------------|-----------------------------------|
| 01 | Ip_VideoStream  | DdsCam::VideoStream      | 影像串流 (此為網路攝影機之特性)      |
| 02 | Ip_Query        | Paas::Cam::Query         | 查詢接口                          |
| 03 | Ip_Jpeg2Ai      | Paas::Cam::JpegType      | H.264 轉換為 Jpeg 格式輸出         |
| 04 | Ip_Ai2Jpeg      | Paas::Cam::JpegType      | Ai render 輸出 Jpeg               |
| 05 | Ip_PlayH264     | Paas::Cam::PlayH264      | Jpeg 轉換為 H.264 格式輸出         |

---

- **DdsCam::TagObjectType**

| No  | Field      | Data Type  | Note         |
| --- | ---------- | ---------- | ------------ |
| 01  | tag_object | string(20) | 推論物件名稱 |

- **DdsCam::VideoStream**
Topic: Tp_VideoStream

| No  | Field           | Data Type               | Note                                                                                             |
| --- | --------------- | ----------------------- | ------------------------------------------------------------------------------------------------ |
| 01  | source          | string(63)              | 發送端識別碼                                                                                     |
| 02  | destination     | string(63)              | 接收端識別碼; 無特定目標, 則為("")空白                                                           |
| 03  | unix_time       | int64                   | 資料發送時間(sec)                                                                                |
| 04  | nanoseconds     | int32                   | 資料發送時間(nanosec)                                                                            |
| 05  | format_code     | int16                   | >0:PaaS規範<br/> <0:自定義<br/> 0:無定義<br/> 1:h264,1920x1080,30fps<br/> 2:h264,1920x1080,60fps |
| 06  | session_number  | uint16                  | 可為遞增整數/亂數                                                                                |
| 07  | flag            | octet                   | 1=Key Frame; 0=Not key frame                                                                     |
| 08  | sequence_number | uint32                  | 串流畫面編號, 遞增整數                                                                           |
| 09  | frame_bytes     | int32                   | 串流畫面資料長度(bytes)                                                                          |
| 10  | frame           | sequence<octet,2097152> | 串流畫面資料; 最大2MiB                                                                           |

- **DdsCam::Query**
Topic: Tp_Query

| No  | Field       | Data Type           | Note                                   |
| --- | ----------- | ------------------- | -------------------------------------- |
| 01  | query_type  | int16               | 0:歷史查詢<br/>1:即時查詢              |
| 02  | source      | string(63)          | 發送端識別碼                           |
| 03  | partition_device | string(63)          | Partition 設備位置，locationxy/uuid |
| 04  | partition_user | string(63)          | Partition token, NCHC 當前tab的訪問token |
| 05  | unix_time_start   | int64               | 請求影片開始時間(sec)                      |
| 06  | unix_time_end   | int64               | 請求影片結束時間(sec)                      |
| 07  | tag_objects | TagObjectType\[10\] | 查詢的推論物件名稱; 最多十項           |
| 08  | activate    | octet               | 1 = streaming is continue <br/> 0 = streaming is stop|

- **DdsCam::PlayH264**
Topic: Tp_PlayH264

| No  | Field           | Data Type               | Note                                                                                             |
| --- | --------------- | ----------------------- | ------------------------------------------------------------------------------------------------ |
| 01  | query_type      | int16                   | 0:歷史查詢<br/>1:即時查詢                                                                        |
| 02  | source          | string(63)              | 發送端識別碼                                                                                     |
| 03  | unix_time       | int64                   | 資料發送時間(sec)                                                                                |
| 04  | nanoseconds     | int32                   | 資料發送時間(nanosec)                                                                            |
| 05  | format_code     | int16                   | >0:PaaS規範<br/> <0:自定義<br/> 0:無定義<br/> 1:h264,1920x1080,30fps<br/> 2:h264,1920x1080,60fps |
| 06  | flag            | octet                   | 1=Key Frame; 0=Not key frame                                                                     |
| 07  | sequence_number | uint32                  | 串流畫面編號, 遞增整數                                                                           |
| 08  | frame_bytes     | int32                   | 串流畫面資料長度(bytes)                                                                          |
| 09  | frame           | sequence<octet,2097152> | 串流畫面資料; 最大2MiB                                                                           |

- **DdsCam::JpegType**
Topic: Tp_JpegType

| No | Field            | Data Type                | Note                              |
|----|------------------|--------------------------|-----------------------------------|
| 01 | query_type       | octet                    | 0:底層查詢<br>1:用戶查詢              |
| 02 | tag_objects      | TagObjectType[10]        | 查詢對應的標物件種類; 最多十項       |
| 03 | source           | string(63)               | 發送端識別碼                        |
| 04 | partition_device | string(256)              | 發送端 Partition; /地面/設備 Id      |
| 05 | partition_user   | string(100)              | 發送端 Partition; 使用者帳號         |
| 06 | unix_time        | int64                    | 格料整數時間(sec)                  |
| 07 | nanoseconds      | int32                    | 格料整數時間(nanosec)              |
| 08 | format_code      | int16                    | >0:Paas 壓縮<br><0:自定義<br>0:無壓縮 |
| 09 | flag             | octet                    | 1=Key Frame; 0=Not key frame      |
| 10 | sequence_number  | uint32                   | 串流流暢編號, 現場標數              |
| 11 | jpeg_bytes       | int32                    | Jpeg 圖面資料長度(bytes)           |
| 12 | jpeg             | sequence<octet,2097152>  | Jpeg 圖面資料; 最大2MiB            |
| 13 | has_ai_meta      | boolean                  | True:有Pre-Ai的meta資料; <br/> False:無;|
| 14 | ai_meta          | sequence<AiMetaType>     | Pre-Ai的meta資料            |

- **Paas::Cam::AiMetaType**

| No | Field       | Data Type   | Note                   |
|----|-------------|-------------|------------------------|
| 01 | tag_object  | string(20)  | 推薦物件名稱            |
| 02 | upper_left_x| int32       | 推薦物件左上角的 x 座標 |
| 03 | upper_left_y| int32       | 推薦物件左上角的 y 座標 |
| 04 | width       | int32       | Jpeg 寬度               |
| 05 | height      | int32       | Jpeg 高度               |
| 06 | confidence  | float       | 推薦信心比例            |

