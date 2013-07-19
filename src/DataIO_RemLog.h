#ifndef INC_DATAIO_REMLOG_H
#define INC_DATAIO_REMLOG_H
#include "DataIO.h"
#include "BufferedLine.h"
// Class: DataIO_RemLog
/// Read replica exchange log data.
class DataIO_RemLog : public DataIO {
  public:
    DataIO_RemLog();
    static DataIO* Alloc() { return (DataIO*)new DataIO_RemLog(); }
    int ReadData(std::string const&,ArgList&,DataSetList&,std::string const&);
    int processWriteArgs(ArgList&) { return 0; }
    int WriteData(std::string const&, DataSetList const&, DimArray const&) { return 1; }
    int WriteData2D(std::string const&, DataSet const&, DimArray const&) { return 1; }
    int WriteData3D(std::string const&, DataSet const&, DimArray const&) { return 1; }
    int WriteDataInverted(std::string const&, DataSetList const &, DimArray const&)
      { return 1; }
    bool ID_DataFormat(CpptrajFile&) { return false; }
  private:
    enum ExchgType { UNKNOWN = 0, TREMD, HREMD, MREMD };
    int ReadRemlogHeader(BufferedLine&, ExchgType&);
    int debug_;
};
#endif
