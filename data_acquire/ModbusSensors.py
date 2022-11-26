import modbus_tk
import modbus_tk.defines as cst
import modbus_tk.modbus_tcp as modbus_tcp

logger = modbus_tk.utils.create_logger("console")

class ModbusSensors:
    def __init__(self, host='127.0.0.1'):
        #Connect to the slave
        self.host = host
        self.connect()

    def connect(self):
        try:
            self.master = modbus_tcp.TcpMaster(host=self.host)
            self.master.set_timeout(5.0)
        except:
            self.master = None
    
    def readSensors(self):
        result = None
        if self.master == None:
            self.connect()
        try:
            data = self.master.execute(1, cst.READ_INPUT_REGISTERS, 100, 10)
            result = [d / 100 for d in data]
        except:
            ...
        return result
        