#pragma once

#define DEFINE_FIELD(field_name, type)											\
struct field_name##_field_t														\
	{																			\
	struct _alias_t																\
	{																			\
		static constexpr const char _literal[] = #field_name;					\
		using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;	\
		template <typename T>													\
		struct _member_t														\
		{																		\
			T field_name;														\
			T& operator()()														\
			{																	\
				return field_name;												\
			}																	\
			const T& operator()() const											\
			{																	\
				return field_name;												\
			}																	\
		};																		\
	};																			\
																				\
	using _traits = sqlpp::make_traits<type>;									\
};																				\

#define DEFINE_TABLE(table_name, ...)											\
struct table_t : sqlpp::table_t<table_t, ##__VA_ARGS__>\
{																				\
	struct _alias_t																\
	{																			\
		static constexpr const char _literal[] = #table_name;					\
		using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;	\
		template <typename T>													\
		struct _member_t														\
		{																		\
			T table_name;														\
			T& operator()()														\
			{																	\
				return table_name;												\
			}																	\
			const T& operator()() const											\
			{																	\
				return table_name;												\
			}																	\
		};																		\
	};																			\
};																				