from contextlib import closing
from threading import Thread
from zipfile import ZipFile
import os
import struct
import tempfile

class EncodeError(Exception):
    def __init__(self, *args, **kwargs):
        super(EncodeError, self).__init__(*args, **kwargs)


class _MeshWriter(object):
    """Appends data to mesh files """
    INT32_MAX = int((1 << 31) - 1)
    INT32_MIN = int(-(1 << 31))
    INT64_MIN = -(1 << 63)
    INT64_MAX = (1 << 63) - 1
    UINT64_MAX = (1 << 64) - 1

    def __init__(self, filename):
        self.file = ZipFile(filename, 'a')
        self._tmpdir = None
        self._pipe_name = None
        self._zipentry_file = None
        self._thread = None

    def WriteNonNegInt(self, value):
        if value < 0: raise EncodeError('Writing negative integer')
        self.AppendVarint32(value)

    def WriteRaw(self, string):
        self._zipentry_file.write(string)

    def WriteString(self, string):
        self.WriteNonNegInt(len(string))
        self._zipentry_file.write(string)

    def WriteProto(self, proto):
        self.WriteString(proto.SerializeToString())

    def WriteSection(self, section_name):
        self._close_zipentry()

        # Create a named pipe
        self._tmpdir = tempfile.mkdtemp()
        self._pipe_name = os.path.join(self._tmpdir, 'myfifo')
        os.mkfifo(self._pipe_name)

        # Copy from the read-end to the zipfile in a different thread
        self._thread = Thread(target=lambda: self.file.write(self._pipe_name, section_name))
        self._thread.start()

        # Open write-end and expose to this class
        self._zipentry_file = open(self._pipe_name, 'w')

    def AppendVarint32(self, value):
        """Appends a signed 32-bit integer to the internal buffer,
        encoded as a varint.  (Note that a negative varint32 will
        always require 10 bytes of space.)
        """
        if not _MeshWriter.INT32_MIN <= value <= _MeshWriter.INT32_MAX:
          raise EncodeError('Value out of range: %d' % value)
        self.AppendVarint64(value)

    def _close_zipentry(self):
        # Close pipe
        if self._zipentry_file:
            self._zipentry_file.close()

        # Wait for thread to complete
        if self._thread:
            self._thread.join()

        # Delete pipe file
        if self._pipe_name:
            os.remove(self._pipe_name)

        # Delete temp directory
        if self._tmpdir:
            os.rmdir(self._tmpdir)

    def close(self):
        self._close_zipentry()
        return self.file.close()

    def AppendVarint64(self, value):
        """Appends a signed 64-bit integer to the internal buffer,
        encoded as a varint.
        """
        if not _MeshWriter.INT64_MIN <= value <= _MeshWriter.INT64_MAX:
          raise EncodeError('Value out of range: %d' % value)
        if value < 0:
          value += (1 << 64)
        self.AppendVarUInt64(value)

    def AppendVarUInt64(self, unsigned_value):
        """Appends an unsigned 64-bit integer to the internal buffer,
        encoded as a varint.
        """
        if not 0 <= unsigned_value <= _MeshWriter.UINT64_MAX:
          raise EncodeError('Value out of range: %d' % unsigned_value)
        while True:
          bits = unsigned_value & 0x7f
          unsigned_value >>= 7
          if not unsigned_value:
            self._zipentry_file.write(struct.pack('B', bits))
            break
          self._zipentry_file.write(struct.pack('B', 0x80|bits))


def MeshWriter(*args, **kwargs):
    return closing(_MeshWriter(*args, **kwargs))
