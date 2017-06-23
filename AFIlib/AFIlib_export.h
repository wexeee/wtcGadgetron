//examplelib_export.h

#ifndef AFI_EXPORT_H_
#define AFI_EXPORT_H_

#if defined (WIN32)
#if defined (gadgetronAFIlib_EXPORTS)
#define EXPORTGADGETSAFI __declspec(dllexport)
#else
#define EXPORTGADGETSAFI __declspec(dllimport)
#endif
#else
#define EXPORTGADGETSAFI
#endif

#endif /* AFI_EXPORT_H_ */
