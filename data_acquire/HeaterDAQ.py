import mysql.connector
from ModbusSensors import ModbusSensors
import time
import datetime


if __name__ == '__main__':
  ms = ModbusSensors('*** Wemos DS18B20 sensor IP address here ***')

  dataBase = mysql.connector.connect(
    host ="*** mysql hostname here ***",
    user ="*** mysql username here ***",
    passwd ="*** mysql password here ***"
  )
  dbName = '*** mysql database here ***'
  dbTable = '*** mysql table here ***'

  # preparing a cursor object
  cursorObject = dataBase.cursor()

  commitCnt = 0
  queryData = []

  while True:
    data = {}
    data['time'] = '\'' + datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S') + '\''
    msData = ms.readSensors()
    if msData:
      data = {**data, **msData}
      data = ", ".join(map(str, data.values()))
      queryData.append(f'({data})')
      time.sleep(10)
      commitCnt += 1
      # Every 1 minute commmit all data at once
      if commitCnt >= 6:
        query = f'INSERT INTO {dbName}.{dbTable}(time, sens_cnt, avg_temp, capacity, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10) VALUES {", ".join(map(str, queryData))};'
        cursorObject.execute(query)
        dataBase.commit()
        commitCnt = 0
        queryData = []

  # Disconnecting from the server
  dataBase.close()
