#pragma once

#define T_INLINE inline
#define T_BEGIN_NAMESPACE namespace tamashii {
#define T_END_NAMESPACE }
#define T_USE_NAMESPACE using namespace tamashii;

#define T_DEFAULT_MOVE_COPY_CONSTRUCTOR(NAME) \
	NAME (const NAME&) = default; \
	NAME & operator=(const NAME&) = default; \
	NAME (NAME&&) = default; \
	NAME & operator=(NAME&&) = default;

#define T_DELETE_MOVE_COPY_CONSTRUCTOR(NAME) \
	NAME (const NAME&) = delete; \
	NAME & operator=(const NAME&) = delete; \
	NAME (NAME&&) = delete; \
	NAME & operator=(NAME&&) = delete;
