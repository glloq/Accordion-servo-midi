#ifndef STATIC_ASSERT_H
#define STATIC_ASSERT_H

// Verification a la compilation, utilisable en C++98 comme en C++11.
//
// L'IDE Arduino compile en gnu++11 depuis longtemps, mais rien ne garantit le dialecte
// choisi par un utilisateur qui recompile a la main : cette version fonctionne dans les
// deux cas, et le nom du message apparait tel quel dans l'erreur du compilateur.
//
//   ACCORDION_STATIC_ASSERT(condition, message_sans_espaces);

#if defined(__cplusplus) && __cplusplus >= 201103L
  #define ACCORDION_STATIC_ASSERT(cond, msg) static_assert((cond), #msg)
#else
  #define ACCORDION_STATIC_ASSERT_CAT2(a, b) a##b
  #define ACCORDION_STATIC_ASSERT_CAT(a, b) ACCORDION_STATIC_ASSERT_CAT2(a, b)
  #define ACCORDION_STATIC_ASSERT(cond, msg) \
      typedef char ACCORDION_STATIC_ASSERT_CAT(msg##_at_line_, __LINE__)[(cond) ? 1 : -1]
#endif

#endif
