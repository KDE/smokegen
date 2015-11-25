static const char Injected[] = R"-(
#if defined(Q_MOC_OUTPUT_REVISION) || defined(Q_MOC_RUN)

#ifdef Q_COMPILER_VARIADIC_MACROS
#define QT_ANNOTATE_CLASS(type, ...) \
    __extension__ _Static_assert(sizeof (#__VA_ARGS__), #type);
#else
#define QT_ANNOTATE_CLASS(type, anotation) \
    __extension__ _Static_assert(sizeof (#anotation), #type);
#endif
#define QT_ANNOTATE_CLASS2(type, a1, a2) \
    __extension__ _Static_assert(sizeof (#a1, #a2), #type);



#ifndef QT_NO_META_MACROS
# if defined(QT_NO_KEYWORDS)
#  define QT_NO_EMIT
# else
#   ifndef QT_NO_SIGNALS_SLOTS_KEYWORDS
#     undef  slots
#     define slots Q_SLOTS
#     undef  signals
#     define signals Q_SIGNALS
#   endif
# endif

# undef  Q_SLOTS
# define Q_SLOTS Q_SLOT
# undef  Q_SIGNALS
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
# define Q_SIGNALS public Q_SIGNAL
#else
# define Q_SIGNALS protected Q_SIGNAL
#endif
# undef  Q_PRIVATE_SLOT
# define Q_PRIVATE_SLOT(d, signature) QT_ANNOTATE_CLASS2(qt_private_slot, d, signature)


#undef Q_CLASSINFO
#undef Q_PLUGIN_METADATA
#undef Q_INTERFACES
#undef Q_PROPERTY
#undef Q_PRIVATE_PROPERTY
#undef Q_REVISION
#undef Q_ENUMS
#undef Q_FLAGS
#undef Q_SCRIPTABLE
#undef Q_INVOKABLE
#undef Q_SIGNAL
#undef Q_SLOT
#undef Q_ENUM
#undef Q_FLAG

#define Q_CLASSINFO(name, value)  __extension__ _Static_assert(sizeof (name, value), "qt_classinfo");
#define Q_PLUGIN_METADATA(x) QT_ANNOTATE_CLASS(qt_plugin_metadata, x)
#define Q_INTERFACES(x) QT_ANNOTATE_CLASS(qt_interfaces, x)
#ifdef Q_COMPILER_VARIADIC_MACROS
#define Q_PROPERTY(...) QT_ANNOTATE_CLASS(qt_property, __VA_ARGS__)
#else
#define Q_PROPERTY(text) QT_ANNOTATE_CLASS(qt_property, text)
#endif
#define Q_PRIVATE_PROPERTY(d, text)  QT_ANNOTATE_CLASS2(qt_private_property, d, text)

#define Q_REVISION(v) __attribute__((annotate("qt_revision:" QT_STRINGIFY2(v))))
#define Q_ENUMS(x) QT_ANNOTATE_CLASS(qt_enums, x)
#define Q_FLAGS(x) QT_ANNOTATE_CLASS(qt_flags, x)
#define Q_ENUM_IMPL(ENUM) \
    friend Q_DECL_CONSTEXPR const QMetaObject *qt_getEnumMetaObject(ENUM) Q_DECL_NOEXCEPT { return &staticMetaObject; } \
    friend Q_DECL_CONSTEXPR const char *qt_getEnumName(ENUM) Q_DECL_NOEXCEPT { return #ENUM; }
#define Q_ENUM(x) Q_ENUMS(x) Q_ENUM_IMPL(x)
#define Q_FLAG(x) Q_FLAGS(x) Q_ENUM_IMPL(x)
#define Q_SCRIPTABLE  __attribute__((annotate("qt_scriptable")))
#define Q_INVOKABLE  __attribute__((annotate("qt_invokable")))
#define Q_SIGNAL __attribute__((annotate("qt_signal")))
#define Q_SLOT __attribute__((annotate("qt_slot")))
#endif // QT_NO_META_MACROS


#undef Q_OBJECT_CHECK
#define Q_OBJECT_CHECK \
    template <typename T> inline void qt_check_for_QOBJECT_macro(const T &_q_argument) const \
    { int i = qYouForgotTheQ_OBJECT_Macro(this, &_q_argument); i = i + 1; } \
    QT_ANNOTATE_CLASS(qt_qobject, "")

#undef Q_GADGET
#define Q_GADGET \
public: \
    static const QMetaObject staticMetaObject; \
private: \
    QT_ANNOTATE_CLASS(qt_qgadget, "")

//for qnamespace.h because Q_MOC_RUN is defined
#if defined(Q_MOC_RUN)
#undef Q_OBJECT
#define Q_OBJECT QT_ANNOTATE_CLASS(qt_qobject, "")
#endif

#undef Q_OBJECT_FAKE
#define Q_OBJECT_FAKE Q_OBJECT QT_ANNOTATE_CLASS(qt_fake, "")

#undef QT_MOC_COMPAT
#define QT_MOC_COMPAT  __attribute__((annotate("qt_moc_compat")))

#endif
)-";
