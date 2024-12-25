#ifndef MACROS_H
#define MACROS_H

#define AUTO_PROPERTY_WRITER(TYPE, NAME, WRITER, DEFAULT)                   \
    Q_PROPERTY(TYPE NAME READ NAME WRITE WRITER NOTIFY NAME##Changed FINAL) \
public:                                                                     \
    TYPE NAME() const                                                       \
    {                                                                       \
        return m_##NAME;                                                    \
    }                                                                       \
                                                                            \
signals:                                                                    \
    Q_SIGNAL void NAME##Changed();                                          \
                                                                            \
private:                                                                    \
    TYPE m_##NAME = DEFAULT;

#define AUTO_PROPERTY(TYPE, NAME, DEFAULT)          \
    AUTO_PROPERTY_WRITER(TYPE, NAME, NAME, DEFAULT) \
public:                                             \
    void NAME(TYPE value)                           \
    {                                               \
        if (m_##NAME == value)                      \
            return;                                 \
        m_##NAME = value;                           \
        emit NAME##Changed();                       \
    }                                               \
                                                    \
private:

#define AUTO_PROPERTY_REF(TYPE, NAME, DEFAULT)      \
    AUTO_PROPERTY_WRITER(TYPE, NAME, NAME, DEFAULT) \
public:                                             \
    void NAME(const TYPE& value)                    \
    {                                               \
        if (m_##NAME == value)                      \
            return;                                 \
        m_##NAME = value;                           \
        emit NAME##Changed();                       \
    }                                               \
                                                    \
private:

#define AUTO_PROPERTY_ADDITIONALWRITER(TYPE, NAME, WRITER, DEFAULT) \
    AUTO_PROPERTY_WRITER(TYPE, NAME, NAME, DEFAULT)                 \
public:                                                             \
    void NAME(TYPE value)                                           \
    {                                                               \
        if (m_##NAME == value)                                      \
            return;                                                 \
        m_##NAME = value;                                           \
        WRITER();                                                   \
        emit NAME##Changed();                                       \
    }                                                               \
                                                                    \
private:

#endif // MACROS_H
