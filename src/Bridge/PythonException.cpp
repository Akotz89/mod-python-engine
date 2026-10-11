#include "PythonException.h"
#include <sstream>

namespace PyEng::Bridge
{
    std::string ExceptionHandler::FormatException()
    {
        if (!PyErr_Occurred())
            return "";

        PyObject* excType = nullptr;
        PyObject* excValue = nullptr;
        PyObject* excTraceback = nullptr;

        // Fetch and normalize exception
        PyErr_Fetch(&excType, &excValue, &excTraceback);
        PyErr_NormalizeException(&excType, &excValue, &excTraceback);

        std::ostringstream oss;

        if (excTraceback)
            oss << FormatTraceback(excTraceback);

        oss << FormatExceptionOnly(excType, excValue);

        Py_XDECREF(excType);
        Py_XDECREF(excValue);
        Py_XDECREF(excTraceback);

        // Formatting an owner's exception may itself raise (for example a broken __str__).
        PyErr_Clear();
        return oss.str();
    }

    std::string ExceptionHandler::FormatTraceback(PyObject* traceback)
    {
        if (!traceback)
            return "";

        std::ostringstream oss;
        oss << "Traceback (most recent call last):\n";

        // Iterate through traceback chain
        for (PyTracebackObject* tb = (PyTracebackObject*)traceback; tb != nullptr; tb = tb->tb_next)
        {
            if (tb->tb_frame)
                oss << FormatFrame((PyObject*)tb->tb_frame);
        }

        return oss.str();
    }

    std::string ExceptionHandler::FormatExceptionOnly(PyObject* excType, PyObject* excValue)
    {
        std::ostringstream oss;

        oss << ExtractTypeName(excType);

        if (excValue)
        {
            std::string message = SafeStr(excValue);
            if (!message.empty())
                oss << ": " << message;
        }

        return oss.str();
    }

    std::string ExceptionHandler::ExtractTypeName(PyObject* excType)
    {
        if (!excType)
            return "UnknownException";

        PyObject* typeName = PyObject_GetAttrString(excType, "__name__");
        if (!typeName)
        {
            PyErr_Clear();
            return "UnknownException";
        }

        std::string result = SafeStr(typeName);
        Py_DECREF(typeName);

        return result.empty() ? "UnknownException" : result;
    }

    std::string ExceptionHandler::FormatFrame(PyObject* frame)
    {
        if (!frame)
            return "";

        PyCodeObject* code = PyFrame_GetCode((PyFrameObject*)frame);
        if (!code)
        {
            PyErr_Clear();
            return "";
        }

        // Extract filename
        char const* filenameStr = PyUnicode_AsUTF8(code->co_filename);
        std::string filename = filenameStr ? filenameStr : "<unknown>";
        if (!filenameStr)
            PyErr_Clear();

        // Extract function name
        char const* nameStr = PyUnicode_AsUTF8(code->co_name);
        std::string funcName = nameStr ? nameStr : "<unknown>";
        if (!nameStr)
            PyErr_Clear();

        // Get line number
        int lineNumber = PyFrame_GetLineNumber((PyFrameObject*)frame);

        Py_DECREF(code);

        std::ostringstream oss;
        oss << "  File \"" << filename << "\", line " << lineNumber << ", in " << funcName << "\n";

        return oss.str();
    }

    std::string ExceptionHandler::SafeStr(PyObject* obj)
    {
        if (!obj)
            return "";

        PyObject* str = PyObject_Str(obj);
        if (!str)
        {
            PyErr_Clear();
            return "<exception message unavailable>";
        }

        char const* cstr = PyUnicode_AsUTF8(str);
        std::string result = cstr ? cstr : "<exception message unavailable>";
        if (!cstr)
            PyErr_Clear();
        Py_DECREF(str);

        return result;
    }

} // namespace PyEng::Bridge
