//DREAMlib_export.h

#ifndef DREAM_EXPORT_H_
#define DREAM_EXPORT_H_

#if defined (WIN32)
#if defined (gadgetronDREAMlib_EXPORTS)
#define EXPORTGADGETSDREAM __declspec(dllexport)
#else
#define EXPORTGADGETSDREAM __declspec(dllimport)
#endif
#else
#define EXPORTGADGETSDREAM
#endif

#endif /* DREAM_EXPORT_H_ */
