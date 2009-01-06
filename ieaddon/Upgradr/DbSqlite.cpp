/*----------------------------------------------------------------------------*
 * DbSQLite.h
 *
 * 6 APR 2005
 *
 * This source code is derived from the SQLiteWrapper source produced by
 * rene.nyffenegger@adp-gmbh.ch.  I altered the original source while
 * packaging it for use with Microsoft MFC.
 *
 * The primary objective in this exercise was to make the wrapper
 * suitable for both MCBS and Unicode because Unicdoe is native to the
 * Windows CE OS. There are three key differences between this wrapper
 * and the author's original source.
 *
 * First, all std::string variables were converted to Microsoft's generic
 * string pointers using LPCTSTR.  This should be familiar to developer's
 * accustomed to working with MFC.  This also means that you should rely
 * upon the the Microsoft TEXT or _T macros for hard-coded character strings.
 *
 * Second, I have change the class SQLiteStatement to CSqlStatement and
 * SQLiteWrapper to CDbSQLite.  This was primarily a matter of preference
 * since most MFC developer's recoginize Microsoft's convention for the
 * CFunction nomenclature.
 *
 * Finally, I have added the header SQLite3i.h with typedefs to the various
 * sqlite3.h functions.  These type definitions are "internal" accessors
 * to the sqlite3 functions utilizing either the UTF-8 or UTF-16 variation
 * as appropriate.
 *
 * The remaining comments at the beginning of this file are the author's
 * original copyright message.
 *----------------------------------------------------------------------------*
 * SQLiteWrapper.h
 *
 * Copyright (C) 2004 Ren� Nyffenegger
 *
 * This source code is provided 'as-is', without any express or implied
 * warranty. In no event will the author be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this source code must not be misrepresented; you must not
 *    claim that you wrote the original source code. If you use this source code
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original source code.
 *
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * Ren� Nyffenegger rene.nyffenegger@adp-gmbh.ch
 *----------------------------------------------------------------------------*/
#include "stdafx.h"
#include "DbSQLite.h"
#include "Database.h"

#define DO_SQL_TRACES 1

#ifdef TEST
#undef TEST
#endif
#define TEST(code) ((code)>SQLITE_OK && (code<SQLITE_ROW))

#ifdef GUARD
#undef GUARD
#endif
#define GUARD(x) if (TEST(x)) SQL_ERROR;

#define SQL_ERROR SQL_ERROR_BODY((sqlite3*)GetRoot().GetRawDB(), _sqlite3_errcode((sqlite3*)GetRoot().GetRawDB()), m_szText)

//
// Default ctor.
//
CSqlStatement::CSqlStatement()
{
   m_stmt = NULL;
}

CSqlStatement::CSqlStatement(LPCTSTR statement, sqlite3* db)
{
   m_stmt = NULL;
   GUARD(_sqlite3_prepare(db, statement, -1, &m_stmt, 0));
}
//
// Do we really want to finalize or do we want to leave that up to
// the user?
//
CSqlStatement::~CSqlStatement()
{
   if (m_stmt)
   {
      sqlite3_finalize(m_stmt);
		m_stmt = NULL;
   }
}

bool
CSqlStatement::Bind(int pos_zero_indexed, LPCTSTR value)
{
   GUARD(_sqlite3_bind_text(
		m_stmt, 
		pos_zero_indexed+1, 
		value, 
		(int)_tcslen(value)*2, 
		SQLITE_TRANSIENT));
   return true;
}

bool
CSqlStatement::Bind(int pos_zero_indexed, double value)
{
   GUARD(_sqlite3_bind_double(m_stmt, pos_zero_indexed+1,value));
   return true;
}

bool
CSqlStatement::Bind(int pos_zero_indexed, int value)
{
   GUARD(_sqlite3_bind_int(m_stmt, pos_zero_indexed+1, value));
   return true;
}

bool
CSqlStatement::BindNull(int pos_zero_indexed)
{
   GUARD(_sqlite3_bind_null(m_stmt, pos_zero_indexed+1));
   return true;
}

bool
CSqlStatement::Execute()
{
   int rc = _sqlite3_step(m_stmt);
   if (TEST(rc)) SQL_ERROR;
   if (rc<SQLITE_DONE) return false;
   GUARD(_sqlite3_reset(m_stmt));
   return true;
}

int
CSqlStatement::Fields()
{
   return _sqlite3_column_count(m_stmt);
}

LPCTSTR
CSqlStatement::FieldName(int pos_zero_indexed)
{
   m_szText = LPCTSTR(_sqlite3_column_name(m_stmt, pos_zero_indexed));
   return m_szText;
}

CSqlStatement::EDataType
CSqlStatement::DataType(int pos_zero_indexed)
{
   return EDataType(_sqlite3_column_type(m_stmt, pos_zero_indexed));
}

LPCTSTR
CSqlStatement::FieldType(int pos_zero_indexed)
{
   int dt = _sqlite3_column_type(m_stmt, pos_zero_indexed);

   switch (dt)
   {
      case SQLITE_INTEGER :
           m_szText = _T("INTEGER");
           break;
      case SQLITE_FLOAT:
           m_szText = _T("FLOAT");
           break;
      case SQLITE_TEXT :
           m_szText = _T("TEXT");
           break;
      case SQLITE_BLOB :
           m_szText = _T("BLOBL");
           break;
      case SQLITE_NULL :
           m_szText = _T("NULL");
           break;
   }

   return m_szText;
}

int
CSqlStatement::ValueInt(int pos_zero_indexed)
{
   return _sqlite3_column_int(m_stmt, pos_zero_indexed);
}

LPCTSTR
CSqlStatement::ValueString(int pos_zero_indexed)
{
   return (LPCTSTR)_sqlite3_column_text(m_stmt, pos_zero_indexed);
}

bool
CSqlStatement::RestartSelect()
{
   GUARD(_sqlite3_reset(m_stmt));
   return true;
}

bool
CSqlStatement::Reset()
{
	int rc = _sqlite3_step(m_stmt);
   if (TEST(rc)) SQL_ERROR;
   GUARD(_sqlite3_reset(m_stmt));
   if (rc == SQLITE_ROW) return true;
   return false;
}

bool
CSqlStatement::NextRow()
{
   int rc = _sqlite3_step(m_stmt);

   if (rc==SQLITE_ROW) return true;
   if (rc==SQLITE_DONE)
   {
      GUARD(_sqlite3_reset(m_stmt));
      return false;
   }
	SQL_ERROR;
}
//
// ctor
//
CDbSQLite::CDbSQLite()
{
   m_db = NULL;
}

CDbSQLite::~CDbSQLite()
{
}

bool
CDbSQLite::Open(LPCTSTR db_file)
{
   m_szName = db_file;
   GUARD(_sqlite3_open(db_file, &m_db));
#if defined _DEBUG && defined DO_SQL_TRACES
	_sqlite3_trace(m_db, &TraceCallback, this);
#endif
   return true;
} 

bool 
CDbSQLite::Close()
{
	if (m_db)
	{
		sqlite3_close(m_db);
		m_db = NULL;
		return true;
	}
	return false;
}

//
// Corrected memory leak.
//
bool
CDbSQLite::DirectStatement(LPCTSTR stmt)
{
   CSqlStatement* pStatement = this->Statement(stmt);
   bool fResult = pStatement->Execute();
   delete pStatement;
   return fResult;
}

bool
CDbSQLite::SelectStatement(LPCTSTR stmt, ResultTable& res)
{
   LPSTR lpsz = NULL;
   size_t len = 0;
   int rc = 0;
   char* errmsg = NULL;
   bool fResult = TRUE;

   res.reset();

   len = _tcslen(stmt);
   lpsz = (LPSTR) LocalAlloc(LMEM_ZEROINIT, len+2);

   // A UTF-16 version of sqlite_exec does not exist at this time,
   // so we will convert the string if necessary.

#if defined(_UNICODE) || defined(UNICODE)
   ::WideCharToMultiByte(CP_ACP, 0, stmt, -1, lpsz, (int)len, 0, 0);
#else
   memcpy(lpsz, stmt, len);
#endif

   rc = sqlite3_exec(m_db,lpsz,SelectCallback,static_cast<void*>(&res),&errmsg);

   if ( rc != SQLITE_OK )
   {
      fResult = false;
   }

   LocalFree(lpsz);
   return fResult;
}

// TODO parameter p_col_names
int
CDbSQLite::SelectCallback(void *p_data, int num_fields, char **p_fields, char**  /*p_col_names*/)
{
   ResultTable* res = reinterpret_cast<ResultTable*>(p_data);

   ResultRecord record;

   for ( int i=0; i < num_fields; i++ )
   {
      CString szField = CString(p_fields[i]);
      record.m_fields.push_back(szField);
   }

   res->m_records.push_back(record);

   return 0;
}

void 
CDbSQLite::TraceCallback(void* obj, const char* text)
{
	CA2T text16(text);
	TRACE_I(FS(_T("SQL[%08X]: %s"), obj, text16));
}

CSqlStatement*
CDbSQLite::Statement(LPCTSTR statement)
{
   CSqlStatement* stmt = NULL;
   stmt = new CSqlStatement(statement, m_db);
   return stmt;
}

LPCTSTR
CDbSQLite::LastError()
{
   m_szText = (LPCTSTR)_sqlite3_errmsg(m_db);
   return m_szText;
}

bool
CDbSQLite::Begin()
{
   m_szText = _T("begin");
   return DirectStatement(m_szText);
}

bool
CDbSQLite::Commit()
{
   m_szText = _T("commit");
   return DirectStatement(m_szText);
}

bool
CDbSQLite::Rollback()
{
   m_szText = _T("rollback");
   return DirectStatement(m_szText);
}