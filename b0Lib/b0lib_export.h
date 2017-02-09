//examplelib_export.h

#ifndef EXAMPLE_EXPORT_H_
#define EXAMPLE_EXPORT_H_

#if defined (WIN32)
#if defined (gadgetronexamplelib_EXPORTS)
#define EXPORTGADGETSEXAMPLE __declspec(dllexport)
#else
#define EXPORTGADGETSEXAMPLE __declspec(dllimport)
#endif
#else
#define EXPORTGADGETSEXAMPLE
#endif

#endif /* EXAMPLE_EXPORT_H_ */
