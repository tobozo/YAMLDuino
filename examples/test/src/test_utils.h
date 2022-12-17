#pragma once

int test_number = 1;

typedef void(test_fn_t)();

static int free_heap_begin;
static int free_heap_end;


struct testreport_t
{
  int test_number;
  const char *fn_name;
  const char* desc;
  const char* usage;
  int free_heap_begin;
  int free_heap_end;
  int free_heap_diff;
};

#include <vector>
std::vector<testreport_t> test_reports;

const char* test_decorator_begin   = "▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼▼";
const char* test_decorator_fail    = "  ✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖✖";
const char* test_decorator_success = "  ✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓✓";


void test_fn( test_fn_t fn, const char *fn_name, const char* desc, const char* usage )
{
  {
    free_heap_begin = HEAP_AVAILABLE();
    Serial.println();
    Serial.println(test_decorator_begin);
    Serial.printf( "\n  [TEST #%d]\n\t* Type: %s()\n\t* Desc: %s\n\t* Usage: %s\n\n", test_number, fn_name, desc, usage );
  }
  {
    // scope the function
    fn();
  }
  {
    free_heap_end = HEAP_AVAILABLE();
    int free_heap_diff = free_heap_begin;
    free_heap_diff -= free_heap_end;
    Serial.println("\n");
    if( free_heap_diff > 0 ) {
      // uh-oh mem leak
      Serial.println(test_decorator_fail);
      Serial.printf ("  ✖ [Memleak] ✖ Bytes diff=%-6d ✖ Before=%-6d, After=%-6d ✖\n", free_heap_diff, free_heap_begin, free_heap_end );
      Serial.println(test_decorator_fail);
    } else {
      Serial.println(test_decorator_success);
      Serial.println("                            Tests completeted successfully                            ");
      Serial.println(test_decorator_success);
    }

    test_reports.push_back({
      test_number,
      fn_name,
      desc,
      usage,
      free_heap_begin,
      free_heap_end,
      free_heap_diff
    });

    Serial.println();
    test_number++;
  }
}


const char* fn_names[3] =
{
  "serializeYml", "deserializeYml", "gettext"
};



void printReport( const char* fn_name )
{
  for( int i=0; i<test_reports.size(); i++ ) {
    testreport_t t = test_reports[i];
    if( strcmp( t.fn_name, fn_name ) != 0 ) continue;
    if( t.free_heap_diff !=0 ) {
      Serial.printf( "  [✖] TEST #%-2d: ⚠ %d bytes mem leak: %s using %s() FAILED\n", t.test_number, t.free_heap_diff, t.desc, t.fn_name );
    } else {
      Serial.printf( "  [✓] TEST #%-2d: %s using %s() succeeded\n", t.test_number, t.desc, t.fn_name );
    }
  }
}


void printGlobalReport()
{
  Serial.println(test_decorator_begin);
  Serial.println();
  for( int i=0;i<3;i++ ) {
    Serial.printf("-- %s --\n", fn_names[i] );
    printReport( fn_names[i] );
  }
  Serial.println();
}

