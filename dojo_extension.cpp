#define DUCKDB_EXTENSION_MAIN

#include "dojo_extension.hpp"

#include "duckdb.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/common/types/value.hpp"
#include "duckdb/function/scalar_function.hpp"
#include "duckdb/function/table_function.hpp"
#include "duckdb/main/client_context.hpp"
#include "duckdb/main/connection.hpp"
#include "duckdb/main/extension_util.hpp"

#include <algorithm>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace duckdb {

struct DojoTask {
	int32_t task_id;
	int32_t level;
	std::string title;
	std::string topic;
	int32_t difficulty;
	std::string goal;
	std::string badge;
	bool requires_order;
	int32_t max_rows; // -1 means no max
	std::vector<std::string> expected_columns;
	std::string expected_sql; // canonical
	std::vector<std::string> hints;
};

static const std::vector<DojoTask> &GetTasks() {
	static const std::vector<DojoTask> tasks = {
    {
      1,
      1,
      "The Yellow Sprint",
      "Filtering + Sorting + Limiting",
      1,
      R"DOJO(Find the names of ducklings that are yellow and younger than 5. Sort by age, youngest first. Show only the first 3 names.)DOJO",
      "WHERE + AND + ORDER BY + LIMIT",
      true,
      3,
      { "name" },
      R"DOJO(SELECT name
FROM ducklings
WHERE color = 'yellow'
  AND age < 5
ORDER BY age ASC, name ASC
LIMIT 3;)DOJO",
      { R"DOJO(Start with SELECT name FROM ducklings)DOJO", R"DOJO(Filter with WHERE color = 'yellow')DOJO", R"DOJO(Add age condition with AND age < 5)DOJO", R"DOJO(Sort with ORDER BY age ASC)DOJO", R"DOJO(Cap results with LIMIT 3)DOJO" }
    },
    {
      2,
      2,
      "Youngest in the Flock",
      "Sorting + Top-1",
      1,
      R"DOJO(Which duck is the youngest overall? Show their name only. Sort by age, youngest first, return only one row.)DOJO",
      "ORDER BY ASC",
      true,
      1,
      { "name" },
      R"DOJO(SELECT name
FROM ducklings
ORDER BY age ASC, name ASC
LIMIT 1;)DOJO",
      { R"DOJO(SELECT name FROM ducklings)DOJO", R"DOJO(Sort by smallest age: ORDER BY age ASC)DOJO", R"DOJO(Return one row: LIMIT 1)DOJO" }
    },
    {
      3,
      3,
      "Elder of the Pond",
      "Sorting + Top-1",
      1,
      R"DOJO(Find the oldest duck in the pond and show their name only. Sort by age, oldest first, return only one row.)DOJO",
      "ORDER BY DESC",
      true,
      1,
      { "name" },
      R"DOJO(SELECT name
FROM ducklings
ORDER BY age DESC, name ASC
LIMIT 1;)DOJO",
      { R"DOJO(SELECT name FROM ducklings)DOJO", R"DOJO(Sort by largest age: ORDER BY age DESC)DOJO", R"DOJO(Return one row: LIMIT 1)DOJO" }
    },
    {
      4,
      4,
      "Golden Roll Call",
      "Filtering + Sorting by Another Column",
      2,
      R"DOJO(Find yellow ducklings that are age 3 or older. Sort by name A to Z. Show only the first 2 names.)DOJO",
      "Sorting by a different column",
      true,
      2,
      { "name" },
      R"DOJO(SELECT name
FROM ducklings
WHERE color = 'yellow'
  AND age >= 3
ORDER BY name ASC
LIMIT 2;)DOJO",
      { R"DOJO(SELECT name FROM ducklings)DOJO", R"DOJO(WHERE color = 'yellow')DOJO", R"DOJO(Add age threshold: AND age >= 3)DOJO", R"DOJO(Sort alphabetically: ORDER BY name ASC)DOJO", R"DOJO(LIMIT 2)DOJO" }
    },
    {
      5,
      5,
      "Fastest Waddlers",
      "Top-N",
      2,
      R"DOJO(Show the names of the 4 youngest ducklings, any color. Sort by age, youngest first.)DOJO",
      "Top-N results",
      true,
      4,
      { "name" },
      R"DOJO(SELECT name
FROM ducklings
ORDER BY age ASC, name ASC
LIMIT 4;)DOJO",
      { R"DOJO(SELECT name FROM ducklings)DOJO", R"DOJO(ORDER BY age ASC)DOJO", R"DOJO(LIMIT 4)DOJO" }
    },
    {
      6,
      6,
      "The Between Pond",
      "Range Filtering",
      2,
      R"DOJO(Find ducklings aged 2 through 4 (inclusive). Sort by age, youngest first. Show only the first 5 names.)DOJO",
      "Range filtering (BETWEEN)",
      true,
      5,
      { "name" },
      R"DOJO(SELECT name
FROM ducklings
WHERE age BETWEEN 2 AND 4
ORDER BY age ASC, name ASC
LIMIT 5;)DOJO",
      { R"DOJO(SELECT name FROM ducklings)DOJO", R"DOJO(Range filter: WHERE age >= 2 AND age <= 4)DOJO", R"DOJO(Alternative (also valid): WHERE age BETWEEN 2 AND 4)DOJO", R"DOJO(ORDER BY age ASC)DOJO", R"DOJO(LIMIT 5)DOJO" }
    },
    {
      7,
      7,
      "Count the Yellow",
      "Aggregates",
      3,
      R"DOJO(How many ducklings are yellow? Return a single number.)DOJO",
      "COUNT(*)",
      false,
      1,
      { "count" },
      R"DOJO(SELECT COUNT(*) AS count
FROM ducklings
WHERE color = 'yellow';)DOJO",
      { R"DOJO(Use an aggregate: SELECT COUNT(*) AS count)DOJO", R"DOJO(From the table: FROM ducklings)DOJO", R"DOJO(Filter to yellow: WHERE color = 'yellow')DOJO" }
    },
    {
      8,
      8,
      "Color Census",
      "GROUP BY",
      3,
      R"DOJO(Show how many ducklings there are of each color. Return color and count, highest count first.)DOJO",
      "GROUP BY + Aggregates",
      false,
      4,
      { "color", "count" },
      R"DOJO(SELECT color, COUNT(*) AS count
FROM ducklings
GROUP BY color
ORDER BY count DESC, color ASC;)DOJO",
      { R"DOJO(Select the group key + aggregate: SELECT color, COUNT(*) AS count)DOJO", R"DOJO(Group rows: FROM ducklings GROUP BY color)DOJO", R"DOJO(Sort by count: ORDER BY count DESC)DOJO" }
    },
    {
      9,
      9,
      "Popular Colors Only",
      "HAVING",
      4,
      R"DOJO(Show only colors that have at least 3 ducklings. Return color and count, highest count first.)DOJO",
      "HAVING",
      false,
      2,
      { "color", "count" },
      R"DOJO(SELECT color, COUNT(*) AS count
FROM ducklings
GROUP BY color
HAVING COUNT(*) >= 3
ORDER BY count DESC, color ASC;)DOJO",
      { R"DOJO(Start from Level 8 (group by color with count))DOJO", R"DOJO(Filter groups using HAVING COUNT(*) >= 3)DOJO", R"DOJO(ORDER BY count DESC)DOJO" }
    },
    {
      10,
      10,
      "The Approved Palette",
      "IN",
      4,
      R"DOJO(Show duckling names where color is either yellow or brown. Sort by name A to Z.)DOJO",
      "IN (...)",
      true,
      9,
      { "name" },
      R"DOJO(SELECT name
FROM ducklings
WHERE color IN ('yellow', 'brown')
ORDER BY name ASC;)DOJO",
      { R"DOJO(SELECT name FROM ducklings)DOJO", R"DOJO(Use IN: WHERE color IN ('yellow', 'brown'))DOJO", R"DOJO(ORDER BY name ASC)DOJO" }
    },
    {
      11,
      11,
      "Names That Start With D",
      "LIKE",
      4,
      R"DOJO(Find ducklings whose names start with the letter “D”. Sort by name A to Z.)DOJO",
      "LIKE patterns",
      true,
      3,
      { "name" },
      R"DOJO(SELECT name
FROM ducklings
WHERE name LIKE 'D%'
ORDER BY name ASC;)DOJO",
      { R"DOJO(SELECT name FROM ducklings)DOJO", R"DOJO(Pattern match: WHERE name LIKE 'D%')DOJO", R"DOJO(ORDER BY name ASC)DOJO" }
    },
    {
      12,
      12,
      "Age Rank Parade",
      "Window Functions",
      5,
      R"DOJO(Show each duckling’s name, age, and their rank by age (youngest = 1). Sort by rank, then name.)DOJO",
      "Window Functions (ROW_NUMBER)",
      true,
      12,
      { "name", "age", "age_rank" },
      R"DOJO(SELECT
  name,
  age,
  ROW_NUMBER() OVER (ORDER BY age ASC, name ASC) AS age_rank
FROM ducklings
ORDER BY age_rank ASC, name ASC;)DOJO",
      { R"DOJO(You need a window function: ROW_NUMBER() OVER (ORDER BY age ASC))DOJO", R"DOJO(Select fields: SELECT name, age, ROW_NUMBER() OVER (...) AS age_rank)DOJO", R"DOJO(Sort results: ORDER BY age_rank ASC, name ASC)DOJO" }
    }
	};
	return tasks;
}

static const DojoTask *FindTask(int32_t task_id) {
	auto &tasks = GetTasks();
	for (auto &t : tasks) {
		if (t.task_id == task_id) {
			return &t;
		}
	}
	return nullptr;
}

static std::string DucklingsSetupSQL() {
	return R"DOJO(-- duckdb_dojo v0.1 starter dataset
-- Run this in your session before attempting dojo levels:
--   .read ducklings.sql
-- or copy/paste into DuckDB.
--
-- If we implement dojo_setup() in the extension, it will execute the equivalent.

CREATE OR REPLACE TEMP TABLE ducklings (
  name  VARCHAR,
  color VARCHAR,
  age   INTEGER
);

INSERT INTO ducklings (name, color, age) VALUES
  ('Daffy',   'yellow', 1),
  ('Goldie',  'yellow', 2),
  ('Daisy',   'yellow', 3),
  ('Puddles', 'brown',  4),
  ('Waddles', 'green',  5),
  ('Beakley', 'brown',  6),
  ('Mallow',  'green',  7),
  ('Nugget',  'yellow', 8),
  ('Duke',    'brown',  9),
  ('Splash',  'blue',  10),
  ('Moss',    'brown', 11),
  ('Sunny',   'yellow', 12);

-- Sanity check:
-- SELECT * FROM ducklings ORDER BY age;)DOJO";
}

struct MaterializedRows {
	std::vector<std::string> col_names;
	std::vector<std::vector<std::string>> rows; // rows[col] stringified
};

static MaterializedRows Materialize(QueryResult &result) {
	MaterializedRows out;
	out.col_names = result.names;

	while (true) {
		auto chunk = result.Fetch();
		if (!chunk || chunk->size() == 0) {
			break;
		}
		auto row_count = chunk->size();
		auto col_count = chunk->ColumnCount();
		for (idx_t r = 0; r < row_count; r++) {
			std::vector<std::string> row;
			row.reserve(col_count);
			for (idx_t c = 0; c < col_count; c++) {
				row.push_back(chunk->GetValue(c, r).ToString());
			}
			out.rows.push_back(std::move(row));
		}
	}
	return out;
}

static std::string JoinRow(const std::vector<std::string> &row) {
	// Unit Separator for low collision risk
	const char sep = 0x1f;
	std::string s;
	for (idx_t i = 0; i < row.size(); i++) {
		if (i) s.push_back(sep);
		s += row[i];
	}
	return s;
}

static std::vector<std::string> NormalizeColNames(const std::vector<std::string> &cols) {
	std::vector<std::string> out;
	out.reserve(cols.size());
	for (auto &c : cols) {
		out.push_back(StringUtil::Lower(c));
	}
	return out;
}

static bool EqualCols(const std::vector<std::string> &a, const std::vector<std::string> &b) {
	if (a.size() != b.size()) return false;
	for (idx_t i = 0; i < a.size(); i++) {
		if (StringUtil::Lower(a[i]) != StringUtil::Lower(b[i])) return false;
	}
	return true;
}

static std::string ColList(const std::vector<std::string> &cols) {
	std::ostringstream ss;
	for (idx_t i = 0; i < cols.size(); i++) {
		if (i) ss << ", ";
		ss << cols[i];
	}
	return ss.str();
}

static unique_ptr<QueryResult> SafeQuery(Connection &con, const std::string &sql, std::string &err) {
	auto res = con.Query(sql);
	if (!res) {
		err = "Unknown error executing query.";
		return nullptr;
	}
	if (res->HasError()) {
		err = res->GetError();
		return nullptr;
	}
	return res;
}

static void EnsureDucklings(Connection &con) {
	// We reset the TEMP ducklings table on every check to ensure deterministic tasks.
	std::string err;
	auto res = SafeQuery(con, DucklingsSetupSQL(), err);
	if (!res) {
		// If setup fails, let the calling code return a message. Throwing here simplifies flow.
		throw InvalidInputException("Failed to initialize ducklings dataset: %s", err);
	}
}

static std::string CompareResults(const DojoTask &task, const MaterializedRows &expected,
                                  const MaterializedRows &actual, bool &ok_out) {
	// Column shape checks
	if (!EqualCols(actual.col_names, task.expected_columns)) {
		ok_out = false;
		std::ostringstream ss;
		ss << "Column mismatch. Expected columns: [" << ColList(task.expected_columns) << "], got: ["
		   << ColList(actual.col_names) << "].";
		ss << " Tip: use aliases (AS ...) to match expected column names.";
		return ss.str();
	}
	if (task.max_rows >= 0 && (int32_t)actual.rows.size() > task.max_rows) {
		ok_out = false;
		std::ostringstream ss;
		ss << "Too many rows. This level expects at most " << task.max_rows << " row(s), but your query returned "
		   << actual.rows.size() << ".";
		ss << " Tip: use LIMIT " << task.max_rows << ".";
		return ss.str();
	}

	// Prepare comparisons
	if (expected.rows.size() != actual.rows.size()) {
		ok_out = false;
		std::ostringstream ss;
		ss << "Row count mismatch. Expected " << expected.rows.size() << " row(s), got " << actual.rows.size() << ".";
		ss << " Tip: check your WHERE / GROUP BY / LIMIT logic.";
		return ss.str();
	}

	auto exp_joined = std::vector<std::string>();
	auto act_joined = std::vector<std::string>();
	exp_joined.reserve(expected.rows.size());
	act_joined.reserve(actual.rows.size());
	for (auto &r : expected.rows) exp_joined.push_back(JoinRow(r));
	for (auto &r : actual.rows) act_joined.push_back(JoinRow(r));

	if (!task.requires_order) {
		std::sort(exp_joined.begin(), exp_joined.end());
		std::sort(act_joined.begin(), act_joined.end());
	}

	for (idx_t i = 0; i < exp_joined.size(); i++) {
		if (exp_joined[i] != act_joined[i]) {
			ok_out = false;
			std::ostringstream ss;
			ss << "Result mismatch. Your output does not match the expected result.";
			if (task.requires_order) {
				ss << " This level checks ordering, so make sure to include the ORDER BY from the goal.";
			} else {
				ss << " Note: this level does not require ordering.";
			}
			ss << " First difference at row " << (i + 1) << ".";
			return ss.str();
		}
	}

	ok_out = true;
	return "✅ Nice work, your query matches the expected output for this level.";
}


// -------------------------- dojo_setup (table function) --------------------------

struct DojoSetupBindData : public TableFunctionData {
	bool done = false;
};

static unique_ptr<FunctionData> DojoSetupBind(ClientContext &context, TableFunctionBindInput &input,
                                             vector<LogicalType> &return_types, vector<string> &names) {
	(void)context;
	(void)input;
	return_types.clear();
	names.clear();
	return_types.push_back(LogicalType::VARCHAR);
	names.push_back("message");
	return make_uniq<DojoSetupBindData>();
}

static void DojoSetupFunc(ClientContext &context, TableFunctionInput &input, DataChunk &output) {
	auto &bind = input.bind_data->Cast<DojoSetupBindData>();
	if (bind.done) {
		output.SetCardinality(0);
		return;
	}

	bind.done = true;
	output.SetCardinality(1);

	string message;
	try {
		Connection con(*context.db);
		string err;
		auto res = SafeQuery(con, DucklingsSetupSQL(), err);
		if (!res) {
			message = "Failed to create ducklings dataset: " + err;
		} else {
			message = "Ducklings dataset loaded as TEMP table 'ducklings' (name, color, age).";
		}
	} catch (std::exception &ex) {
		message = std::string("Internal exception: ") + ex.what();
	}

	output.SetValue(0, 0, Value(message));
}

// -------------------------- dojo_tasks (table function) --------------------------

struct DojoTasksState : public FunctionData {
	idx_t offset = 0;
};

static unique_ptr<FunctionData> DojoTasksBind(ClientContext &context, TableFunctionBindInput &input,
                                             vector<LogicalType> &return_types, vector<string> &names) {
	(void)context;
	(void)input;
	return_types = {
	    LogicalType::INTEGER, // task_id
	    LogicalType::INTEGER, // level
	    LogicalType::VARCHAR, // title
	    LogicalType::VARCHAR, // topic
	    LogicalType::INTEGER, // difficulty
	    LogicalType::VARCHAR, // goal
	    LogicalType::VARCHAR, // badge
	    LogicalType::BOOLEAN, // requires_order
	    LogicalType::INTEGER, // max_rows
	    LogicalType::LIST(LogicalType::VARCHAR) // expected_columns
	};
	names = {
	    "task_id",
	    "level",
	    "title",
	    "topic",
	    "difficulty",
	    "goal",
	    "badge",
	    "requires_order",
	    "max_rows",
	    "expected_columns"
	};
	return make_uniq<DojoTasksState>();
}

static void DojoTasksFunc(ClientContext &context, TableFunctionInput &data_p, DataChunk &output) {
	(void)context;
	auto &state = data_p.bind_data->Cast<DojoTasksState>();
	auto &tasks = GetTasks();

	idx_t row = 0;
	while (state.offset < tasks.size() && row < STANDARD_VECTOR_SIZE) {
		auto &t = tasks[state.offset];

		output.SetValue(0, row, Value::INTEGER(t.task_id));
		output.SetValue(1, row, Value::INTEGER(t.level));
		output.SetValue(2, row, Value(t.title));
		output.SetValue(3, row, Value(t.topic));
		output.SetValue(4, row, Value::INTEGER(t.difficulty));
		output.SetValue(5, row, Value(t.goal));
		output.SetValue(6, row, Value(t.badge));
		output.SetValue(7, row, Value::BOOLEAN(t.requires_order));
		output.SetValue(8, row, Value::INTEGER(t.max_rows));

		std::vector<Value> col_vals;
		col_vals.reserve(t.expected_columns.size());
		for (auto &c : t.expected_columns) {
			col_vals.push_back(Value(c));
		}
		output.SetValue(9, row, Value::LIST(LogicalType::VARCHAR, col_vals));

		state.offset++;
		row++;
	}
	output.SetCardinality(row);
}

// -------------------------- dojo_hint (scalar) --------------------------

static void DojoHintFunc(DataChunk &args, ExpressionState &state, Vector &result) {
	(void)state;

	result.SetVectorType(VectorType::FLAT_VECTOR);
	auto result_data = FlatVector::GetData<string_t>(result);

	auto &task_id_vec = args.data[0];
	auto &hint_level_vec = args.data[1];

	UnifiedVectorFormat task_id_data;
	UnifiedVectorFormat hint_level_data;
	task_id_vec.ToUnifiedFormat(args.size(), task_id_data);
	hint_level_vec.ToUnifiedFormat(args.size(), hint_level_data);

	for (idx_t i = 0; i < args.size(); i++) {
		auto task_idx = task_id_data.sel->get_index(i);
		auto hint_idx = hint_level_data.sel->get_index(i);

		if (!task_id_data.validity.RowIsValid(task_idx) || !hint_level_data.validity.RowIsValid(hint_idx)) {
			result_data[i] = StringVector::AddString(result, "NULL input is not allowed.");
			continue;
		}

		auto task_id = ((int32_t *)task_id_data.data)[task_idx];
		auto hint_level = ((int32_t *)hint_level_data.data)[hint_idx];

		auto task = FindTask(task_id);
		if (!task) {
			result_data[i] = StringVector::AddString(result, "Unknown task_id. Try: SELECT * FROM dojo_tasks();");
			continue;
		}
		if (hint_level < 1) {
			result_data[i] = StringVector::AddString(result, "Hint levels start at 1.");
			continue;
		}
		if ((idx_t)hint_level > task->hints.size()) {
			std::ostringstream ss;
			ss << "No more hints. This level has " << task->hints.size() << " hint(s).";
			result_data[i] = StringVector::AddString(result, ss.str());
			continue;
		}

		result_data[i] = StringVector::AddString(result, task->hints[(idx_t)hint_level - 1]);
	}

	FlatVector::SetNull(result, false);
}

// -------------------------- dojo_check (table function) --------------------------

struct DojoCheckState : public FunctionData {
	int32_t task_id;
	std::string user_sql;
};

static unique_ptr<FunctionData> DojoCheckBind(ClientContext &context, TableFunctionBindInput &input,
                                             vector<LogicalType> &return_types, vector<string> &names) {
	(void)context;
	if (input.inputs.size() != 2) {
		throw InvalidInputException("dojo_check requires 2 arguments: task_id (INTEGER), user_sql (VARCHAR)");
	}
	auto task_id = input.inputs[0].GetValue<int32_t>();
	auto user_sql = input.inputs[1].ToString();

	auto task = FindTask(task_id);
	if (!task) {
		throw InvalidInputException("Unknown task_id %d. Try: SELECT * FROM dojo_tasks();", task_id);
	}

	return_types = {
	    LogicalType::BOOLEAN, // ok
	    LogicalType::VARCHAR, // message
	    LogicalType::UBIGINT, // expected_rows
	    LogicalType::UBIGINT  // actual_rows
	};
	names = {"ok", "message", "expected_rows", "actual_rows"};

	auto state = make_uniq<DojoCheckState>();
	state->task_id = task_id;
	state->user_sql = user_sql;
	return state;
}

static void DojoCheckFunc(ClientContext &context, TableFunctionInput &data_p, DataChunk &output) {
	auto &state = data_p.bind_data->Cast<DojoCheckState>();
	auto task = FindTask(state.task_id);
	D_ASSERT(task);

	bool ok = false;
	std::string message;

	try {
		Connection con(*context.db);

		EnsureDucklings(con);

		std::string err;
		auto expected_res = SafeQuery(con, task->expected_sql, err);
		if (!expected_res) {
			ok = false;
			message = "Internal error: failed to compute expected result: " + err;
		} else {
			auto expected_mat = Materialize(*expected_res);

			auto actual_res = SafeQuery(con, state.user_sql, err);
			if (!actual_res) {
				ok = false;
				std::ostringstream ss;
				ss << "Your query failed to run: " << err;
				ss << " Try: SELECT dojo_hint(" << task->task_id << ", 1);";
				message = ss.str();
				MaterializedRows empty;
				output.SetValue(0, 0, Value::BOOLEAN(ok));
				output.SetValue(1, 0, Value(message));
				output.SetValue(2, 0, Value::UBIGINT(expected_mat.rows.size()));
				output.SetValue(3, 0, Value::UBIGINT(0));
				output.SetCardinality(1);
				return;
			}

			auto actual_mat = Materialize(*actual_res);

			// Replace actual column names with expected list for shape check against spec,
			// because the user might not alias, and we want to provide a helpful error.
			// We still validate names; we don't auto-fix.
			message = CompareResults(*task, expected_mat, actual_mat, ok);

			output.SetValue(0, 0, Value::BOOLEAN(ok));
			output.SetValue(1, 0, Value(message));
			output.SetValue(2, 0, Value::UBIGINT(expected_mat.rows.size()));
			output.SetValue(3, 0, Value::UBIGINT(actual_mat.rows.size()));
			output.SetCardinality(1);
			return;
		}
	} catch (std::exception &ex) {
		ok = false;
		message = std::string("Internal exception: ") + ex.what();
	}

	output.SetValue(0, 0, Value::BOOLEAN(ok));
	output.SetValue(1, 0, Value(message));
	output.SetValue(2, 0, Value::UBIGINT(0));
	output.SetValue(3, 0, Value::UBIGINT(0));
	output.SetCardinality(1);
}

// -------------------------- extension load --------------------------

static void LoadInternal(ExtensionLoader &loader) {
	// dojo_setup()
	TableFunction setup_fun("dojo_setup", {}, DojoSetupFunc, DojoSetupBind);
	loader.RegisterFunction(setup_fun);

	// dojo_tasks()
	TableFunction tasks_fun("dojo_tasks", {}, DojoTasksFunc, DojoTasksBind);
	loader.RegisterFunction(tasks_fun);

	// dojo_hint(task_id, hint_level)
	ScalarFunction hint_fun("dojo_hint", {LogicalType::INTEGER, LogicalType::INTEGER}, LogicalType::VARCHAR, DojoHintFunc);
	loader.RegisterFunction(hint_fun);

	// dojo_check(task_id, user_sql)
	TableFunction check_fun("dojo_check", {LogicalType::INTEGER, LogicalType::VARCHAR}, DojoCheckFunc, DojoCheckBind);
	loader.RegisterFunction(check_fun);
}

void DojoExtension::Load(ExtensionLoader &loader) {
	LoadInternal(loader);
}

std::string DojoExtension::Name() {
	return "dojo";
}

std::string DojoExtension::Version() const {
#ifdef EXT_VERSION_DOJO
	return EXT_VERSION_DOJO;
#else
	return "";
#endif
}

} // namespace duckdb

extern "C" {

DUCKDB_CPP_EXTENSION_ENTRY(dojo, loader) {
	duckdb::LoadInternal(loader);
}

}
