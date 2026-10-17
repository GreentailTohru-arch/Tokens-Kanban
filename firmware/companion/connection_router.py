"""Single-sender USB/BLE selection, with bounded retries and safe USB promotion."""
import time


class ConnectionRouter:
    def __init__(self, mode, candidates, open_usb, open_ble, port=None,
                 ble_address=None, clock=time.monotonic, retry_seconds=30):
        self.mode, self.candidates = mode, candidates
        self.open_usb, self.open_ble = open_usb, open_ble
        self.port, self.ble_address = port, ble_address
        self.clock, self.retry_seconds = clock, retry_seconds
        self.retry_at = {}

    @staticmethod
    def close(connection):
        if connection:
            try:
                connection.close()
            except OSError:
                pass

    def failed(self, connection):
        if connection and not str(connection.port).startswith('BLE'):
            self.retry_at[connection.port] = self.clock() + self.retry_seconds
        self.close(connection)

    def ensure(self, current, allow_upgrade=True):
        # Explicit modes stay pinned. Only auto can promote an active BLE link.
        if current and (self.mode != 'auto' or not str(current.port).startswith('BLE')
                        or not allow_upgrade or self.ble_address):
            return current
        ports = self.candidates(self.port) if self.mode != 'ble' else []
        self.retry_at = {p: t for p, t in self.retry_at.items() if p in ports}
        if len(ports) > 1:
            if current:
                return current
            raise OSError('Multiple USB devices; select --port COMx')
        error = None
        if ports and self.clock() >= self.retry_at.get(ports[0], 0):
            try:
                replacement = self.open_usb(ports[0])
            except OSError as exc:
                error = exc
                self.retry_at[ports[0]] = self.clock() + self.retry_seconds
            else:
                # The replacement has completed its handshake before BLE closes.
                self.close(current)
                return replacement
        if current:
            return current
        if self.mode == 'usb' or self.port:
            raise error or OSError('Waiting for available Passport USB connection')
        connection = self.open_ble(self.ble_address)
        return connection
