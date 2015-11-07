#ifndef OGR_INCLUDE_HPP
#define OGR_INCLUDE_HPP

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable : 4458)
# pragma warning(disable : 4251)
#else
# pragma GCC diagnostic push
# ifdef __clang__
#  pragma GCC diagnostic ignored "-Wdocumentation-unknown-command"
# endif
# pragma GCC diagnostic ignored "-Wfloat-equal"
# pragma GCC diagnostic ignored "-Wold-style-cast"
# pragma GCC diagnostic ignored "-Wpadded"
# pragma GCC diagnostic ignored "-Wredundant-decls"
# pragma GCC diagnostic ignored "-Wshadow"
#endif

/* Strictly speaking the following include would be enough here,
   but everybody using this file will very likely need the other includes,
   so we are adding them here, so that not everybody will need all those
   pragmas to disable warnings. */
//#include <ogr_geometry.h>
#include <ogr_api.h>
#include <ogrsf_frmts.h>

#ifdef _MSC_VER
# pragma warning(pop)
#else
# pragma GCC diagnostic pop
#endif

#if GDAL_VERSION_MAJOR < 2
struct OGRDataSourceDestroyer {
    void operator()(OGRDataSource* ptr) {
        if (ptr) {
            OGRDataSource::DestroyDataSource(ptr);
        }
    }
};
#else
struct GDALDatasetDestroyer {
    void operator()(GDALDataset* ptr) {
        if (ptr) {
            GDALClose(ptr);
        }
    }
};
#endif

#endif // OGR_INCLUDE_HPP
