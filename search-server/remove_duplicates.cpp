#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {
	SearchServer& inside = search_server;
	std::set<std::set<std::string>> no_dupl_words;
	std::vector<int> del_id;
	for (const auto document_id : inside) {
		std::set<std::string> doc_words;
		for (auto [word, _] : search_server.GetWordFrequencies(document_id)){
			doc_words.insert(word);
		}
		if (no_dupl_words.count(doc_words)) {
			del_id.push_back(document_id);
		}
		else {
			no_dupl_words.insert(doc_words);
		}
		doc_words.clear();
	}
	for (const auto id : del_id) {
		std::cout << "Found duplicate document "s << id << std::endl;
		search_server.RemoveDocument(id);
	}
}
