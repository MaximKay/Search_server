#include "read_input_functions.h"
#include "string_processing.h"
#include "document.h"
#include "search_server.h"
#include "paginator.h"
#include "request_queue.h"
#include "test_example_functions.h"
#include "remove_duplicates.h"

using namespace std::string_literals;

int main() {
	/*{
		TestSearchServer();
		std::cout << "All tests OK"s << std::endl;
	}*/

	/*{
		SearchServer search_server("and in at"s);
		RequestQueue request_queue(search_server);

		search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, {7, 2, 7});
		search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, {1, 2, 3});
		search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, {1, 2, 8});
		search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, {1, 3, 2});
		search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, {1, 1, 1});

		// 1439 empty requests
		for (int i = 0; i < 1439; ++i) {
			request_queue.AddFindRequest("empty request"s);
		}
		// still 1439 empty requests
		request_queue.AddFindRequest("curly dog"s);
		// 1438 empty requests
		request_queue.AddFindRequest("big collar"s);
		// 1437 empty requests
		request_queue.AddFindRequest("sparrow"s);
		std::cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << std::endl;
	}*/

	/*{
		SearchServer search_server("� � ��"s);

		AddDocument(search_server, 1, "�������� ��� �������� �����"s, DocumentStatus::ACTUAL, {7, 2, 7});
		AddDocument(search_server, 1, "�������� �� � ������ �������"s, DocumentStatus::ACTUAL, {1, 2});
		AddDocument(search_server, -1, "�������� �� � ������ �������"s, DocumentStatus::ACTUAL, {1, 2});
		AddDocument(search_server, 3, "������� �� ����\x12��� �������"s, DocumentStatus::ACTUAL, {1, 3, 2});
		AddDocument(search_server, 4, "������� �� ������� �������"s, DocumentStatus::ACTUAL, {1, 1, 1});

		FindTopDocuments(search_server, "�������� -��"s);
		FindTopDocuments(search_server, "�������� --���"s);
		FindTopDocuments(search_server, "�������� -"s);

		MatchDocuments(search_server, "�������� ��"s);
		MatchDocuments(search_server, "������ -���"s);
		MatchDocuments(search_server, "������ --��"s);
		MatchDocuments(search_server, "�������� - �����"s);
	}*/

	/*{
		SearchServer search_server("and with"s);

		search_server.AddDocument(1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, {7, 2, 7});
		search_server.AddDocument(2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2, 3});
		search_server.AddDocument(3, "big cat nasty hair"s, DocumentStatus::ACTUAL, {1, 2, 8});
		search_server.AddDocument(4, "big dog cat Vladislav"s, DocumentStatus::ACTUAL, {1, 3, 2});
		search_server.AddDocument(5, "big dog hamster Borya"s, DocumentStatus::ACTUAL, {1, 1, 1});

		const auto search_results = search_server.FindTopDocuments("curly dog"s);
		int page_size = 2;
		const auto pages = Paginate(search_results, page_size);

		// pagination
		for (auto page = pages.begin(); page != pages.end(); ++page) {
			std::cout << *page << std::endl;
			std::cout << "Page break"s << std::endl;
		}
	}*/

	{
		SearchServer search_server("and with"s);

		AddDocument(search_server, 1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, {7, 2, 7});
		AddDocument(search_server, 2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

		// �������� ��������� 2, ����� �����
		AddDocument(search_server, 3, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

		// ������� ������ � ����-������, ������� ����������
		AddDocument(search_server, 4, "funny pet and curly hair"s, DocumentStatus::ACTUAL, {1, 2});

		// ��������� ���� ����� ��, ������� ���������� ��������� 1
		AddDocument(search_server, 5, "funny funny pet and nasty nasty rat"s, DocumentStatus::ACTUAL, {1, 2});

		// ���������� ����� �����, ���������� �� ��������
		AddDocument(search_server, 6, "funny pet and not very nasty rat"s, DocumentStatus::ACTUAL, {1, 2});

		// ��������� ���� ����� ��, ��� � id 6, �������� �� ������ �������, ������� ����������
		AddDocument(search_server, 7, "very nasty rat and not very funny pet"s, DocumentStatus::ACTUAL, {1, 2});

		// ���� �� ��� �����, �� �������� ����������
		AddDocument(search_server, 8, "pet with rat and rat and rat"s, DocumentStatus::ACTUAL, {1, 2});

		// ����� �� ������ ����������, �� �������� ����������
		AddDocument(search_server, 9, "nasty rat with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

		std::cout << "Before duplicates removed: "s << search_server.GetDocumentCount() << std::endl;
		RemoveDuplicates(search_server);
		std::cout << "After duplicates removed: "s << search_server.GetDocumentCount() << std::endl;
	}

	return 0;
}
