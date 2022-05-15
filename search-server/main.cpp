// -------- Начало модульных тестов поисковой системы ----------

ostream& operator << (ostream &out,const DocumentStatus &status){
    switch (status) {
        case DocumentStatus::ACTUAL:
            out << "ACTUAL"s;
            break;
        case DocumentStatus::BANNED:
            out << "BANNED"s;
            break;
        case DocumentStatus::REMOVED:
            out << "REMOVED"s;
            break;
        case DocumentStatus::IRRELEVANT:
            out << "IRRELEVANT"s;
            break;
    }
    return out;
}
// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        ASSERT_EQUAL(found_docs[0].id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
    }
}

void TestAddedDocumentsAreSearchedByRequest() {
    const int doc_id1 = 12;
    const string content1 = "one red shoe found under a shelf"s;
    const vector<int> ratings1 = {1, 2, 3};

    const int doc_id2 = 15;
    const string content2 = "green hat found on the table"s;
    const vector<int> ratings2 = {3, 3, 5};

        SearchServer server;
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc_id2, content2, DocumentStatus::BANNED, ratings2);
        {
            const auto found_docs = server.FindTopDocuments("found"s, DocumentStatus::BANNED);
            ASSERT_EQUAL(found_docs.size(), 1);
            ASSERT_EQUAL(found_docs[0].id, doc_id2);
        }
        {
            const auto found_docs = server.FindTopDocuments("found"s, DocumentStatus::ACTUAL);
            ASSERT_EQUAL(found_docs.size(), 1);
            ASSERT_EQUAL(found_docs[0].id, doc_id1);
        }
        {
            server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
            const auto found_docs = server.FindTopDocuments("chair"s);
            ASSERT(found_docs.empty());
        }

}

void TestDocumentsWithMinusWordsExcludeFromResult(){
    const int doc_id1 = 12;
    const string content1 = "one red shoe found under a shelf"s;
    const vector<int> ratings1 = {1, 2, 3};

    const int doc_id2 = 15;
    const string content2 = "green hat found on the table"s;
    const vector<int> ratings2 = {3, 3, 5};


    SearchServer server;
    server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
    server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
    auto found_docs = server.FindTopDocuments("found -hat"s);
    ASSERT_EQUAL(found_docs.size(), 1);
    ASSERT_EQUAL(found_docs[0].id, doc_id1);

    found_docs = server.FindTopDocuments("found -shoe"s);
    ASSERT_EQUAL(found_docs.size(), 1);
    ASSERT_EQUAL(found_docs[0].id, doc_id2);

    found_docs = server.FindTopDocuments("-found shoe hat"s);
    ASSERT(found_docs.empty());

    found_docs = server.FindTopDocuments("-fdds"s);
    ASSERT(found_docs.empty());
}

void TestMatchingDocuments(){
    const int doc_id1 = 12;
    const string content1 = "one red shoe found under a shelf"s;
    const vector<int> ratings1 = {1, 2, 3};

    const int doc_id2 = 15;
    const string content2 = "green hat found on the table"s;
    const vector<int> ratings2 = {3, 3, 5};

    SearchServer server;
    server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
    server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
    {
        const string raw_query = "green found table"s;
        const auto found_words_with_documents_status = server.MatchDocument(raw_query, doc_id2);
        auto [words, documents_status] = found_words_with_documents_status;
        ASSERT_EQUAL(words.size(), 3);
        vector <string> words_by_query = {"green"s, "found"s, "table"s};
        sort(words_by_query.begin(), words_by_query.end());
        sort(words.begin(), words.end());
        ASSERT_EQUAL(words_by_query, words);
        ASSERT_EQUAL(documents_status, DocumentStatus::ACTUAL);
    }

    {
        const string raw_query = "green found -table"s;
        const auto found_words_with_documents_status = server.MatchDocument(raw_query, doc_id2);
        const auto &[words, documents_status] = found_words_with_documents_status;
        ASSERT(words.empty());
    }

    {
        const string raw_query = "-whisper green found"s;
        const auto found_words_with_documents_status = server.MatchDocument(raw_query, doc_id2);
        auto [words, documents_status] = found_words_with_documents_status;
        ASSERT_EQUAL(words.size(), 2);
        vector <string> words_by_query = {"green"s, "found"s};
        sort(words_by_query.begin(), words_by_query.end());
        sort(words.begin(), words.end());
        ASSERT_EQUAL(words_by_query, words);
        ASSERT_EQUAL(documents_status, DocumentStatus::ACTUAL);
    }
}

void TestSortByRelevance(){
    const int doc_id1 = 12;
    const string content1 = "one red shoe found under a shelf near the table"s;
    const vector<int> ratings1 = {1, 2, 3};

    const int doc_id2 = 15;
    const string content2 = "green hat found on the table"s;
    const vector<int> ratings2 = {3, 3, 5};

    const int doc_id3 = 18;
    const string content3 = "orange cat lost in the forest"s;
    const vector<int> ratings3 = {3, 3, 5};

    const double ERROR_IN_DOUBLE_NUMBERS = 1e-6;
    SearchServer server;
    const string raw_query = "found"s;
    server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
    server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
    server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings3);
    const auto found_docs = server.FindTopDocuments(raw_query);
    ASSERT_EQUAL(found_docs.size(), 2);
    ASSERT(abs(found_docs[0].relevance - 0.0675775) <= ERROR_IN_DOUBLE_NUMBERS);
    ASSERT(abs(found_docs[1].relevance - 0.0405465) <= ERROR_IN_DOUBLE_NUMBERS);
}

void TestDocumentsRatingCompute(){
    const int doc_id1 = 12;
    const string content1 = "one red shoe found under a shelf near the table"s;
    const vector<int> ratings1 = {1, 2, 3};

    const int doc_id2 = 15;
    const string content2 = "green hat found on the table"s;
    const vector<int> ratings2 = {3, 3, 5};

    const int doc_id3 = 18;
    const string content3 = "orange cat lost in the forest"s;
    const vector<int> ratings3 = {1, 1, 1};

    SearchServer server;
    const string raw_query = "the"s;
    server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
    server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
    server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings3);
    const auto found_docs = server.FindTopDocuments(raw_query);
    ASSERT_EQUAL(found_docs.size(), 3);
    ASSERT_EQUAL(found_docs[0].rating, 3);
    ASSERT_EQUAL(found_docs[1].rating, 2);
    ASSERT_EQUAL(found_docs[2].rating, 1);
}

void TestFilterByUserPredicateFunction(){
    const int doc_id1 = 12;
    const string content1 = "one red shoe found under a shelf near the table"s;
    const vector<int> ratings1 = {1, 2, 3};

    const int doc_id2 = 15;
    const string content2 = "green hat found on the table"s;
    const vector<int> ratings2 = {3, 3, 5};

    const int doc_id3 = 18;
    const string content3 = "orange cat lost in the forest"s;
    const vector<int> ratings3 = {1, 1, 1};

    SearchServer server;
    const string raw_query = "the"s;
    server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
    server.AddDocument(doc_id2, content2, DocumentStatus::BANNED, ratings2);
    server.AddDocument(doc_id3, content3, DocumentStatus::REMOVED, ratings3);

    {
        const auto found_docs = server.FindTopDocuments(raw_query,
                                                        [](int document_id, DocumentStatus status, int rating) {
                                                            return document_id != 12;
                                                        });
        ASSERT_EQUAL(found_docs.size(), 2);
        ASSERT_EQUAL(found_docs[0].id, 15);
        ASSERT_EQUAL(found_docs[0].relevance, 0);
        ASSERT_EQUAL(found_docs[0].rating, 3);
        ASSERT_EQUAL(found_docs[1].id, 18);
        ASSERT_EQUAL(found_docs[1].relevance, 0);
        ASSERT_EQUAL(found_docs[1].rating, 1);
    }
    {
        const auto found_docs = server.FindTopDocuments(raw_query,
                                                        [](int document_id, DocumentStatus status, int rating){
                                                            return status!=DocumentStatus::BANNED;
                                                        });
        ASSERT_EQUAL(found_docs.size(), 2);
        ASSERT_EQUAL(found_docs[0].id, 12);
        ASSERT_EQUAL(found_docs[0].relevance, 0);
        ASSERT_EQUAL(found_docs[0].rating, 2);
        ASSERT_EQUAL(found_docs[1].id, 18);
        ASSERT_EQUAL(found_docs[1].relevance, 0);
        ASSERT_EQUAL(found_docs[1].rating, 1);
    }
    {
        const auto found_docs = server.FindTopDocuments(raw_query,
                                                        [](int document_id, DocumentStatus status, int rating){
                                                            return rating < 3;
                                                        });
        ASSERT_EQUAL(found_docs.size(), 2);
        ASSERT_EQUAL(found_docs[0].id, 12);
        ASSERT_EQUAL(found_docs[0].relevance, 0);
        ASSERT_EQUAL(found_docs[0].rating, 2);
        ASSERT_EQUAL(found_docs[1].id, 18);
        ASSERT_EQUAL(found_docs[1].relevance, 0);
        ASSERT_EQUAL(found_docs[1].rating, 1);
    }
}


// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestAddedDocumentsAreSearchedByRequest);
    RUN_TEST(TestDocumentsWithMinusWordsExcludeFromResult);
    RUN_TEST(TestMatchingDocuments);
    RUN_TEST(TestSortByRelevance);
    RUN_TEST(TestDocumentsRatingCompute);
    RUN_TEST(TestFilterByUserPredicateFunction);
}

// --------- Окончание модульных тестов поисковой системы -----------