/****************************************************************************
 *   Copyright (c) 2020 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#ifndef DOCUMENT_PARAMS_H
#define DOCUMENT_PARAMS_H

/*[[[cog
import DocumentParams
DocumentParams.declare()
]]]*/

// Auto generated code (Tools/params_utils.py:82)
#include <Base/Parameter.h>
#include <boost_signals2.hpp>

// Auto generated code (Tools/params_utils.py:90)
namespace App {
/** Convenient class to obtain App::Document related parameters

 * The parameters are under group "User parameter:BaseApp/Preferences/Document"
 *
 * This class is auto generated by App/DocumentParams.py. Modify that file
 * instead of this one, if you want to add any parameter. You need
 * to install Cog Python package for code generation:
 * @code
 *     pip install cogapp
 * @endcode
 *
 * Once modified, you can regenerate the header and the source file,
 * @code
 *     python3 -m cogapp -r App/DocumentParams.h App/DocumentParams.cpp
 * @endcode
 *
 * You can add a new parameter by adding lines in App/DocumentParams.py. Available
 * parameter types are 'Int, UInt, String, Bool, Float'. For example, to add
 * a new Int type parameter,
 * @code
 *     ParamInt(parameter_name, default_value, documentation, on_change=False)
 * @endcode
 *
 * If there is special handling on parameter change, pass in on_change=True.
 * And you need to provide a function implementation in App/DocumentParams.cpp with
 * the following signature.
 * @code
 *     void DocumentParams:on<parameter_name>Changed()
 * @endcode
 */
class AppExport DocumentParams {
public:
    static ParameterGrp::handle getHandle();

    static boost::signals2::signal<void (const char*)> &signalParamChanged();
    static void signalAll();

    // Auto generated code (Tools/params_utils.py:138)
    //@{
    /// Accessor for parameter prefAuthor
    static const std::string & getprefAuthor();
    static const std::string & defaultprefAuthor();
    static void removeprefAuthor();
    static void setprefAuthor(const std::string &v);
    static const char *docprefAuthor();
    //@}

    // Auto generated code (Tools/params_utils.py:138)
    //@{
    /// Accessor for parameter prefSetAuthorOnSave
    static const bool & getprefSetAuthorOnSave();
    static const bool & defaultprefSetAuthorOnSave();
    static void removeprefSetAuthorOnSave();
    static void setprefSetAuthorOnSave(const bool &v);
    static const char *docprefSetAuthorOnSave();
    //@}

    // Auto generated code (Tools/params_utils.py:138)
    //@{
    /// Accessor for parameter prefCompany
    static const std::string & getprefCompany();
    static const std::string & defaultprefCompany();
    static void removeprefCompany();
    static void setprefCompany(const std::string &v);
    static const char *docprefCompany();
    //@}

    // Auto generated code (Tools/params_utils.py:138)
    //@{
    /// Accessor for parameter prefLicenseType
    static const long & getprefLicenseType();
    static const long & defaultprefLicenseType();
    static void removeprefLicenseType();
    static void setprefLicenseType(const long &v);
    static const char *docprefLicenseType();
    //@}

    // Auto generated code (Tools/params_utils.py:138)
    //@{
    /// Accessor for parameter prefLicenseUrl
    static const std::string & getprefLicenseUrl();
    static const std::string & defaultprefLicenseUrl();
    static void removeprefLicenseUrl();
    static void setprefLicenseUrl(const std::string &v);
    static const char *docprefLicenseUrl();
    //@}

    // Auto generated code (Tools/params_utils.py:138)
    //@{
    /// Accessor for parameter CompressionLevel
    static const long & getCompressionLevel();
    static const long & defaultCompressionLevel();
    static void removeCompressionLevel();
    static void setCompressionLevel(const long &v);
    static const char *docCompressionLevel();
    //@}

    // Auto generated code (Tools/params_utils.py:138)
    //@{
    /// Accessor for parameter CheckExtension
    static const bool & getCheckExtension();
    static const bool & defaultCheckExtension();
    static void removeCheckExtension();
    static void setCheckExtension(const bool &v);
    static const char *docCheckExtension();
    //@}

    // Auto generated code (Tools/params_utils.py:138)
    //@{
    /// Accessor for parameter ForceXML
    static const long & getForceXML();
    static const long & defaultForceXML();
    static void removeForceXML();
    static void setForceXML(const long &v);
    static const char *docForceXML();
    //@}

    // Auto generated code (Tools/params_utils.py:138)
    //@{
    /// Accessor for parameter SplitXML
    static const bool & getSplitXML();
    static const bool & defaultSplitXML();
    static void removeSplitXML();
    static void setSplitXML(const bool &v);
    static const char *docSplitXML();
    //@}

    // Auto generated code (Tools/params_utils.py:138)
    //@{
    /// Accessor for parameter PreferBinary
    static const bool & getPreferBinary();
    static const bool & defaultPreferBinary();
    static void removePreferBinary();
    static void setPreferBinary(const bool &v);
    static const char *docPreferBinary();
    //@}

    // Auto generated code (Tools/params_utils.py:138)
    //@{
    /// Accessor for parameter AutoRemoveFile
    static const bool & getAutoRemoveFile();
    static const bool & defaultAutoRemoveFile();
    static void removeAutoRemoveFile();
    static void setAutoRemoveFile(const bool &v);
    static const char *docAutoRemoveFile();
    //@}

    // Auto generated code (Tools/params_utils.py:138)
    //@{
    /// Accessor for parameter BackupPolicy
    static const bool & getBackupPolicy();
    static const bool & defaultBackupPolicy();
    static void removeBackupPolicy();
    static void setBackupPolicy(const bool &v);
    static const char *docBackupPolicy();
    //@}

    // Auto generated code (Tools/params_utils.py:138)
    //@{
    /// Accessor for parameter CreateBackupFiles
    static const bool & getCreateBackupFiles();
    static const bool & defaultCreateBackupFiles();
    static void removeCreateBackupFiles();
    static void setCreateBackupFiles(const bool &v);
    static const char *docCreateBackupFiles();
    //@}

    // Auto generated code (Tools/params_utils.py:138)
    //@{
    /// Accessor for parameter UseFCBakExtension
    static const bool & getUseFCBakExtension();
    static const bool & defaultUseFCBakExtension();
    static void removeUseFCBakExtension();
    static void setUseFCBakExtension(const bool &v);
    static const char *docUseFCBakExtension();
    //@}

    // Auto generated code (Tools/params_utils.py:138)
    //@{
    /// Accessor for parameter SaveBackupDateFormat
    static const std::string & getSaveBackupDateFormat();
    static const std::string & defaultSaveBackupDateFormat();
    static void removeSaveBackupDateFormat();
    static void setSaveBackupDateFormat(const std::string &v);
    static const char *docSaveBackupDateFormat();
    //@}

    // Auto generated code (Tools/params_utils.py:138)
    //@{
    /// Accessor for parameter CountBackupFiles
    static const long & getCountBackupFiles();
    static const long & defaultCountBackupFiles();
    static void removeCountBackupFiles();
    static void setCountBackupFiles(const long &v);
    static const char *docCountBackupFiles();
    //@}

    // Auto generated code (Tools/params_utils.py:138)
    //@{
    /// Accessor for parameter OptimizeRecompute
    static const bool & getOptimizeRecompute();
    static const bool & defaultOptimizeRecompute();
    static void removeOptimizeRecompute();
    static void setOptimizeRecompute(const bool &v);
    static const char *docOptimizeRecompute();
    //@}

    // Auto generated code (Tools/params_utils.py:138)
    //@{
    /// Accessor for parameter CanAbortRecompute
    static const bool & getCanAbortRecompute();
    static const bool & defaultCanAbortRecompute();
    static void removeCanAbortRecompute();
    static void setCanAbortRecompute(const bool &v);
    static const char *docCanAbortRecompute();
    //@}

    // Auto generated code (Tools/params_utils.py:138)
    //@{
    /// Accessor for parameter UseHasher
    static const bool & getUseHasher();
    static const bool & defaultUseHasher();
    static void removeUseHasher();
    static void setUseHasher(const bool &v);
    static const char *docUseHasher();
    //@}

    // Auto generated code (Tools/params_utils.py:138)
    //@{
    /// Accessor for parameter ViewObjectTransaction
    static const bool & getViewObjectTransaction();
    static const bool & defaultViewObjectTransaction();
    static void removeViewObjectTransaction();
    static void setViewObjectTransaction(const bool &v);
    static const char *docViewObjectTransaction();
    //@}

    // Auto generated code (Tools/params_utils.py:138)
    //@{
    /// Accessor for parameter WarnRecomputeOnRestore
    static const bool & getWarnRecomputeOnRestore();
    static const bool & defaultWarnRecomputeOnRestore();
    static void removeWarnRecomputeOnRestore();
    static void setWarnRecomputeOnRestore(const bool &v);
    static const char *docWarnRecomputeOnRestore();
    //@}

    // Auto generated code (Tools/params_utils.py:138)
    //@{
    /// Accessor for parameter NoPartialLoading
    static const bool & getNoPartialLoading();
    static const bool & defaultNoPartialLoading();
    static void removeNoPartialLoading();
    static void setNoPartialLoading(const bool &v);
    static const char *docNoPartialLoading();
    //@}

    // Auto generated code (Tools/params_utils.py:138)
    //@{
    /// Accessor for parameter SaveThumbnail
    static const bool & getSaveThumbnail();
    static const bool & defaultSaveThumbnail();
    static void removeSaveThumbnail();
    static void setSaveThumbnail(const bool &v);
    static const char *docSaveThumbnail();
    //@}

    // Auto generated code (Tools/params_utils.py:138)
    //@{
    /// Accessor for parameter ThumbnailNoBackground
    static const bool & getThumbnailNoBackground();
    static const bool & defaultThumbnailNoBackground();
    static void removeThumbnailNoBackground();
    static void setThumbnailNoBackground(const bool &v);
    static const char *docThumbnailNoBackground();
    //@}

    // Auto generated code (Tools/params_utils.py:138)
    //@{
    /// Accessor for parameter AddThumbnailLogo
    static const bool & getAddThumbnailLogo();
    static const bool & defaultAddThumbnailLogo();
    static void removeAddThumbnailLogo();
    static void setAddThumbnailLogo(const bool &v);
    static const char *docAddThumbnailLogo();
    //@}

    // Auto generated code (Tools/params_utils.py:138)
    //@{
    /// Accessor for parameter ThumbnailSampleSize
    static const long & getThumbnailSampleSize();
    static const long & defaultThumbnailSampleSize();
    static void removeThumbnailSampleSize();
    static void setThumbnailSampleSize(const long &v);
    static const char *docThumbnailSampleSize();
    //@}

    // Auto generated code (Tools/params_utils.py:138)
    //@{
    /// Accessor for parameter ThumbnailSize
    static const long & getThumbnailSize();
    static const long & defaultThumbnailSize();
    static void removeThumbnailSize();
    static void setThumbnailSize(const long &v);
    static const char *docThumbnailSize();
    //@}

    // Auto generated code (Tools/params_utils.py:138)
    //@{
    /// Accessor for parameter DuplicateLabels
    static const bool & getDuplicateLabels();
    static const bool & defaultDuplicateLabels();
    static void removeDuplicateLabels();
    static void setDuplicateLabels(const bool &v);
    static const char *docDuplicateLabels();
    //@}

    // Auto generated code (Tools/params_utils.py:138)
    //@{
    /// Accessor for parameter TransactionOnRecompute
    static const bool & getTransactionOnRecompute();
    static const bool & defaultTransactionOnRecompute();
    static void removeTransactionOnRecompute();
    static void setTransactionOnRecompute(const bool &v);
    static const char *docTransactionOnRecompute();
    //@}

    // Auto generated code (Tools/params_utils.py:138)
    //@{
    /// Accessor for parameter RelativeStringID
    static const bool & getRelativeStringID();
    static const bool & defaultRelativeStringID();
    static void removeRelativeStringID();
    static void setRelativeStringID(const bool &v);
    static const char *docRelativeStringID();
    //@}

    // Auto generated code (Tools/params_utils.py:138)
    //@{
    /// Accessor for parameter HashIndexedName
    ///
    /// Enable special encoding of indexes name in toponaming. Disabled by
    /// default for backward compatibility
    static const bool & getHashIndexedName();
    static const bool & defaultHashIndexedName();
    static void removeHashIndexedName();
    static void setHashIndexedName(const bool &v);
    static const char *docHashIndexedName();
    //@}

    // Auto generated code (Tools/params_utils.py:138)
    //@{
    /// Accessor for parameter EnableMaterialEdit
    static const bool & getEnableMaterialEdit();
    static const bool & defaultEnableMaterialEdit();
    static void removeEnableMaterialEdit();
    static void setEnableMaterialEdit(const bool &v);
    static const char *docEnableMaterialEdit();
    //@}

// Auto generated code (Tools/params_utils.py:178)
}; // class DocumentParams
} // namespace App
//[[[end]]]

#endif // DOCUMENT_PARAMS_H
