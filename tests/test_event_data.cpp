// Exercise the actual packet adapter helpers, including overflow boundaries.
#define PYTHON_ENGINE_PACKET_ADAPTER_TEST
#include "../src/Bindings/Exports/EventDataExport.cpp"
#include <cassert>
#include <iostream>

int main()
{
    using namespace PyEng::EventData;
    Py_Initialize();
    {
        ByteBuffer buffer(0);
        assert(bp::len(BufferBytes(buffer)) == 0);
        bp::object bytes(bp::handle<>(PyBytes_FromStringAndSize("\x01\x02\x03\x04", 4)));
        AppendBytes(buffer, bytes);
        assert(buffer.size() == 4);
        assert(Read<uint32>(buffer) == 0x04030201);
        assert(buffer.rpos() == 4);
        SetReadPosition(buffer, 2);
        auto failsWith = [&](auto operation, PyObject* expected)
        {
            bool rejected = false;
            try { operation(); }
            catch (bp::error_already_set const&)
            {
                assert(PyErr_ExceptionMatches(expected));
                PyErr_Clear();
                rejected = true;
            }
            assert(rejected);
        };
        failsWith([&] { Read<uint32>(buffer); }, PyExc_IndexError);
        assert(buffer.rpos() == 2);
        failsWith([&] { Get<uint32>(buffer, std::numeric_limits<std::size_t>::max()); }, PyExc_IndexError);
        failsWith([&] { Put<uint16>(buffer, 3, 1); }, PyExc_IndexError);
        failsWith([&] { SetReadPosition(buffer, std::numeric_limits<std::size_t>::max()); }, PyExc_IndexError);
        failsWith([&] { AppendBytes(buffer, bp::object(42)); }, PyExc_TypeError);
        auto copy = BufferBytes(buffer);
        Put<uint16>(buffer, 1, 0x0506);
        assert(Get<uint8>(buffer, 1) == 6 && Get<uint8>(buffer, 2) == 5);
        assert(PyBytes_AS_STRING(copy.ptr())[1] == 2);
        bp::object empty(bp::handle<>(PyBytes_FromStringAndSize("", 0)));
        AppendBytes(buffer, empty);
        assert(buffer.size() == 4);
    }
    Py_FinalizeEx();
    std::cout << "PASS: actual packet helpers empty/copy/append/typed read/write, cursor preservation, range/overflow checks, invalid-buffer rejection\n";
}
