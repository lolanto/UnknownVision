#ifndef TYPE_RESTRICTION_H
#define TYPE_RESTRICTION_H
/** 强类型限制，为同一基本数据类型设置别名，
 * 相比直接使用using 或者 typedef，该方式可以防止数据的误传递
 * (参考自google的Draco，draco_index_type.h)
 */
#define ALIAS_INDEX(basicType, name) \
 struct name##tag_name {}; \
 using name = AliasIndex<basicType, name##tag_name>;

template<typename basicType, typename tagStruct>
class AliasIndex {
public:
	using ThisAliasIndex = AliasIndex<basicType, tagStruct>;
	using ValueType = basicType;
public:
	constexpr AliasIndex() : m_value(ValueType()) {}
	constexpr explicit AliasIndex(ValueType value) : m_value(value) {}
	/** 获取实际值 */
	constexpr ValueType value() const { return m_value; }
	/// 基本的比较操作
#define ALIAS_INDEX_COMPARE_OPERATION(operation) \
 constexpr bool operator##operation(const AliasIndex& rhs) const { \
   return m_value operation rhs.m_value; \
 } \
 constexpr bool operator##operation(const ValueType& rhs) const { \
   return m_value operation rhs; \
 }
	
	ALIAS_INDEX_COMPARE_OPERATION(> );
	ALIAS_INDEX_COMPARE_OPERATION(>= );
	ALIAS_INDEX_COMPARE_OPERATION(< );
	ALIAS_INDEX_COMPARE_OPERATION(<= );
	ALIAS_INDEX_COMPARE_OPERATION(== );
	ALIAS_INDEX_COMPARE_OPERATION(!= );
#undef ALIAS_INDEX_COMPARE_OPERATION
	inline ThisAliasIndex &operator++() {
		++m_value;
		return *this;
	}
	inline ThisAliasIndex operator++(int) {
		const ThisAliasIndex ret(m_value);
		++m_value;
		return ret;
	}
	inline ThisAliasIndex &operator--() {
		--m_value;
		return *this;
	}
	inline ThisAliasIndex operator--(int) {
		const ThisAliasIndex ret(m_value);
		--m_value;
		return ret;
	}
	constexpr ThisAliasIndex operator+(const AliasIndex& rhs) const {
		return ThisAliasIndex(m_value + rhs.m_value);
	}
	constexpr ThisAliasIndex operator+(const ValueType& rhs) const {
		return ThisAliasIndex(m_value + rhs);
	}
	const ThisAliasIndex operator-(const AliasIndex& rhs) const {
		return ThisAliasIndex(m_value - rhs.m_value);
	}
	const ThisAliasIndex operator-(const ValueType& rhs) const {
		return ThisAliasIndex(m_value - rhs);
	}

	inline ThisAliasIndex &operator+=(const AliasIndex& rhs) {
		m_value += rhs.m_value;
		return *this;
	}
	inline ThisAliasIndex &operator+=(const ValueType& rhs) {
		m_value += rhs;
		return *this;
	}
	inline ThisAliasIndex &operator-=(const AliasIndex& rhs) {
		m_value -= rhs.m_value;
		return *this;
	}
	inline ThisAliasIndex &operator-=(const ValueType& rhs) {
		m_value -= rhs;
		return *this;
	}
	inline ThisAliasIndex &operator=(const ThisAliasIndex& rhs) {
		m_value = rhs.m_value;
		return *this;
	}
	inline ThisAliasIndex &operator=(const ValueType& rhs) {
		m_value = rhs;
		return *this;
	}
private:
	ValueType m_value;
};

#endif // TYPE_RESTRICTION_H
