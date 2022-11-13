// -------- Начало модульных тестов поисковой системы ----------
void TestAddDocuments() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {//Проверяем что документ добавился и находится по слову
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("cat"s);
        ASSERT_EQUAL(found_docs.size(), 1);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }
    {//Проверяем что не находится по другим словам
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("man"s);
        ASSERT_HINT(found_docs.empty(), "Должен быть пуст, но не пуст");
    }
    {//Проверяем что не находит пустой документ, документ из пробелов, пустой запрос
        SearchServer server;
        server.AddDocument(doc_id, "", DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments(""s);
        ASSERT_HINT(found_docs.empty(), "Должен быть пуст, но не пуст");
    }
    {
        SearchServer server;
        server.AddDocument(doc_id, "     ", DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("  man"s);
        ASSERT_HINT(found_docs.empty(), "Должен быть пуст, но не пуст");
    }
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments(""s);
        ASSERT_HINT(found_docs.empty(), "Должен быть пуст, но не пуст");
    }
}


// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
            "Stop words must be excluded from documents"s);
    }
}

void TestMinusWords() {//Добавляем три документа 
    const int doc1 = 1;
    const string content1 = "cat in the"s;
    const vector<int> ratings1 = { 1, 2, 3 };
    const int doc2 = 2;
    const string content2 = "cat the city"s;
    const vector<int> ratings2 = { 1, 2, 3 };
    const int doc3 = 3;
    const string content3 = "in the city"s;
    const vector<int> ratings3 = { 1, 2, 3 };
    {//Проверяем что документы с минус словами исключаются из выдачи
        SearchServer server;
        server.AddDocument(doc1, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc2, content2, DocumentStatus::ACTUAL, ratings2);
        server.AddDocument(doc3, content3, DocumentStatus::ACTUAL, ratings3);
        const auto found_docs = server.FindTopDocuments("cat in the -city"s);
        ASSERT_EQUAL(found_docs.size(), 1);
    }
    {//Проверяем с запросом слово которое есть во всех документах
        SearchServer server;
        server.AddDocument(doc1, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc2, content2, DocumentStatus::ACTUAL, ratings2);
        server.AddDocument(doc3, content3, DocumentStatus::ACTUAL, ratings3);
        const auto found_docs = server.FindTopDocuments("-the"s);
        ASSERT_HINT(found_docs.empty(), "Должен быть пустой но не пустой");
    }
}

void TestMatchDocument() {
    //Добавляем документ
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {//Проверяем что возвращает все слова из документа по запросу
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        auto [qwr_words, status] = server.MatchDocument("cat in the city"s, doc_id);
        ASSERT_EQUAL(qwr_words.size(), 4);
    }
    {//Проверяем что возвращает пустой вектор есть есть минус слово
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        auto [qwr_words, status] = server.MatchDocument("cat in the -city"s, doc_id);
        ASSERT(qwr_words.empty());
    }
    {//Проверяем что исключает из мэтчинга стоп слова
        SearchServer server;
        server.SetStopWords("in the");
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        auto [qwr_words, status] = server.MatchDocument("cat in the city"s, doc_id);
        ASSERT_EQUAL(qwr_words.size(), 2);
    }
}

void TestRelevance() {
    const int doc1 = 1;
    const string content1 = "cat"s;
    const vector<int> ratings1 = { 1, 2, 3 };
    const int doc2 = 2;
    const string content2 = "cat in the"s;
    const vector<int> ratings2 = { 1, 2, 3 };
    const int doc3 = 3;
    const string content3 = "cat in the city"s;
    const vector<int> ratings3 = { 1, 2, 3 };
    {//Проверяем что релевантность расположена в порядке убывания
        SearchServer server;
        server.AddDocument(doc1, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc2, content2, DocumentStatus::ACTUAL, ratings2);
        server.AddDocument(doc3, content3, DocumentStatus::ACTUAL, ratings3);
        const auto find_docs = server.FindTopDocuments("cat in the city"s);
        ASSERT_HINT(find_docs[0].relevance > find_docs[1].relevance&& find_docs[1].relevance > find_docs[2].relevance, "Должны были в порядке убывания, но видимо нет");

    }
}

void TestRating() {
    const int doc1 = 1;
    const string content1 = "cat in the city"s;
    const vector<int> ratings1 = { 2, 5 , 7 , 9 , 2 };
    const int doc2 = 2;
    const string content2 = "cat in the city"s;
    const vector<int> ratings2 = { -4, -2, -1, -9 };
    const int doc3 = 3;
    const string content3 = "cat in the city"s;
    const vector<int> ratings3 = { 2, -5 , 7 , -9 , 2, 5, -2 };
    {//Проверяем что правильно вычисляется рейтинг документа
        SearchServer server;
        server.AddDocument(doc1, content1, DocumentStatus::ACTUAL, ratings1);
        const auto find_docs = server.FindTopDocuments("cat in the city"s);
        int size = ratings1.size();
        int rait = (2 + 5 + 7 + 9 + 2) / size;
        ASSERT_EQUAL_HINT(find_docs[0].rating, rait, "Должно быть 3, но что то пошло не так");
    }
    {// Тоже самое для отрицательных оценок
        SearchServer server;
        server.AddDocument(doc2, content2, DocumentStatus::ACTUAL, ratings2);
        const auto find_docs = server.FindTopDocuments("cat in the city"s);
         int size = ratings2.size();
        int rait = (-4-2-1-9) / size;
        ASSERT_EQUAL_HINT(find_docs[0].rating, rait, "Должно быть -4, но что то пошло не так");
    }
    {// Для смешанных оценок
        SearchServer server;
        server.AddDocument(doc3, content3, DocumentStatus::ACTUAL, ratings3);
        const auto find_docs = server.FindTopDocuments("cat in the city"s);
        int size = ratings3.size();
        int rait = (2 - 5 + 7 - 9 + 2 + 5 - 2) / size;
        ASSERT_EQUAL_HINT(find_docs[0].rating, rait, "Должно быть 0, но что то пошло не так");
    }

}

void TestPredicate() {
    const int doc1 = 1;
    const string content1 = "cat in the city"s;
    const vector<int> ratings1 = { 1, 2, 3 };
    const int doc2 = 2;
    const string content2 = "cat in the city"s;
    const vector<int> ratings2 = { 1, 2, 3 };
    const int doc3 = 4;
    const string content3 = "cat in the city"s;
    const vector<int> ratings3 = { 1, 2, 3 };
    {//Проверяем выдачу документов с четными id
        SearchServer server;
        server.AddDocument(doc1, content1, DocumentStatus::ACTUAL, ratings1);
        server.AddDocument(doc2, content2, DocumentStatus::ACTUAL, ratings2);
        server.AddDocument(doc3, content3, DocumentStatus::ACTUAL, ratings3);
        const auto find_docs = server.FindTopDocuments("cat in the city"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; });
        ASSERT_EQUAL(find_docs.size(), 2);
        //Проверяем эти самые id
        ASSERT_EQUAL(find_docs[0].id, doc2);
        ASSERT_EQUAL(find_docs[1].id, doc3);
    }
}

void TestStatus() {
    const int doc1 = 1;
    const string content1 = "cat in the city"s;
    const vector<int> ratings1 = { 1, 2, 3 };
    const int doc2 = 2;
    const string content2 = "cat in the city"s;
    const vector<int> ratings2 = { 1, 2, 3 };
    const int doc3 = 3;
    const string content3 = "cat in the city"s;
    const vector<int> ratings3 = { 1, 2, 3 };
    {//Проверяем нахождение документа по статусу
        SearchServer server;
        server.AddDocument(doc1, content1, DocumentStatus::BANNED, ratings1);
        server.AddDocument(doc2, content2, DocumentStatus::ACTUAL, ratings2);
        server.AddDocument(doc3, content3, DocumentStatus::REMOVED, ratings3);
        const auto find_docs = server.FindTopDocuments("cat in the city"s, DocumentStatus::BANNED);
        ASSERT(!find_docs.empty());
        ASSERT_EQUAL(find_docs[0].id, doc1);
        //Проверим по остальным статусам
        const auto find_docs1 = server.FindTopDocuments("cat in the city"s, DocumentStatus::ACTUAL);
        ASSERT(!find_docs1.empty());
        ASSERT_EQUAL(find_docs1[0].id, doc2);
        const auto find_docs2 = server.FindTopDocuments("cat in the city"s, DocumentStatus::REMOVED);
        ASSERT(!find_docs2.empty());
        ASSERT_EQUAL(find_docs2[0].id, doc3);
        //Теперь проверим что не находит документ со статусом которого нет ни у одного документа
        const auto find_docs4 = server.FindTopDocuments("cat in the city"s, DocumentStatus::IRRELEVANT);
        ASSERT(find_docs4.empty());
    }
}

void TestCorrectCalcRelevance() {
    const int doc_id = 0;
    const string content = "белый кот и модный ошейник"s;
    const vector<int> ratings = { 8, -3 };
    const int doc_id1 = 1;
    const string content1 = "ухоженный пёс выразительные глаза"s;
    const vector<int> ratings1 = { 5, -12, 2, 1 };
    {//Проверяем корректность вычисления релевантности
        SearchServer server;
        server.SetStopWords("и в на"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id1, content1, DocumentStatus::ACTUAL, ratings1);
        const auto find_doc = server.FindTopDocuments("пушистый ухоженный кот"s);
        double relevance_doc = 0.173287;
        double accuracy = 1e-6;     
        ASSERT((abs(find_doc[0].relevance - relevance_doc)) < accuracy);
    }
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestAddDocuments);
    RUN_TEST(TestMinusWords);
    RUN_TEST(TestMatchDocument);
    RUN_TEST(TestRelevance);
    RUN_TEST(TestRating);
    RUN_TEST(TestPredicate);
    RUN_TEST(TestStatus);
    RUN_TEST(TestCorrectCalcRelevance);
}
// --------- Окончание модульных тестов поисковой системы -----------