Реализован функционал поискового сервера со следующими возможностями и методами:
  Сервер покрыт юнит-тестами использующими самостоятельно реализованные макросы фреймворка ASSERT_EQUAL и ASSERT_EQUAL_HINT
  При создании объекта-сервера можно опционально указать стоп-слова в виде строки или контейнера с методами .begin() и .end(),
  из добавляемых документов эти слова будут удалены.
  Метод AddDocument для добавления документа в таком формате (число-id документа, текст документа, статус документа, вектор оценок-рейтингов документа),
  id-документа должен быть неотрицательным числом, а текст не должен содержать спецсимволов, иначе выбросится исключение.
  Статус документов может быть 4-х видов: ACTUAL, IRRELEVANT, BANNED и REMOVED.
  Вектор оценок-рейтингов документа пересчитывается в средний ретйинг и присваивается добавляемому документу.
  Метод FindTopDocuments реализует поиск по документам в следующих вариантах: 
    С многопоточной политикой испольнения или без нее, передается первым аргументом
    Далее передается текст запроса, он парсится на отдельные слова, которые в свою очередь не должны содержать спецсимволы. Текст запроса может содержать минус-слова, которые исключат из результата запроса документы содержащие их.
    Можно указать статус искомых документов - если не указать, то по умолчанию будет поиск по актуальным документам.
    Вместо статуса можно передать предикат с любым фильтром учитывающим три параметра: id, статус и рейтинг.
  Для найденных документов вычисляется релевантность.
  Сортировка найденных документов по релевантности, если релевантность равно, то сортируется по рейтингу.
  Во всех методах сервера где это необходимо, выбрасываются исключения при наличии недопустимых спецсимволов в передаваемых текстах и словах,
  неверных id и неправильно заданных стоп и минус-слов или при попытке обратиться к несуществующему документу.
  У сервера есть методы .begin() и .end() позволяющие итерироваться по id всех документов добавленных на сервер.
  Метод GetWordFrequencies для получения частот слов по id документа.
  Метод RemoveDocument для удаления документов из поискового сервера по id документа, при передаче неверного id ничего не произойдет.
  Метод GetDocumentCount для получения количества документов добавленных на сервер.
  Метод MatchDocument принимающий писковый запрос и id документа в котором необходимо провести поиск совпадений. Запрос может иметь минус-слова и не должен содержать спецсимволов. Возвращает кортеж из вектора совпавших слова и статуса документа. Выкидывает исключение std::out_of_range при передаче id отсутствующего на сервере.
  
Функции вне класса:
  PrintMatchDocumentResult при передаче в нее результатов работы метода класса MatchDocuments выводит в консоль результаты в удобном виде
  AddDocument принимает ссылку на объект класса SearchServer и данные документа который надо добавить. Вызывает метод класса AddDocument и обрабатывает исключения которые может он может выбросить.
  FindTopDocuments принимает ссылку на объект класса SearchServer и поисковый запрос. Использует метод класса FindTopDocuments и обрабатывает возможные исключения. Также логгирует и выводит в конце длительность выполнения данной функции.
  MatchDocuments принимает ссылку на объект класса SearchServer и поисковый запрос. Использует метод класса MatchDocuments и обрабатывает возможные исключения. Также логгирует и выводит в конце длительность выполнения данной функции.
  ProcessQueries принимает ссылку на объект класса SearchServer и вектор поисковых запросов. Использует метод класса SearchServer FindTopDocuments для поиска и возвращает вектор из векторов с результатами для каждого поискового запроса.
  ProcessQueriesJoined принимает ссылку на объект класса SearchServer и вектор поисковых запросов. Использует функцию ProcessQueries для поиска по серверу и затем объединяет результаты поиска в один последовательный вектор результатов в порядке аналогичном вектору запросов. Возвращает вектор из результатов.

Вне класса разработаны:
Функция RemoveDuplicates принимающая ссылку на поисковый сервер для поиска и удаления дубликатов. Она находит и сохраняет id дубликатов и только потом происходит
удаление с помощью метода поискового сервера.

Реализован вспомогательный класс RequestQueue принимающий запросы на поиск в метод AddFindRequest. Данный класс позволяет вернуть значение, 
равное количеству запросов за последние сутки, на которые ничего не нашлось.

Класс Paginator и его вспомогательный IteratorRange. Данные класса реализуют возможность деления результатов поиска на страницы с помощью функции Paginate.
В нее передаются ссылка на объект SearchServer и количество результатов на странице. Возвращает вектор объектов IteratorRange. Оператор вывода перегружен, чтобы вывод осуществлялся простым разыменованием итератора возвращаемого вектора.

Класс LogDuration для логгирования длительности работы любых фрагментов кода. Отсчет начинается с момента создания объекта LogDuration и до вызова его деструктора. По умолчанию при выводе не указвается имя лога, а также результат выводится в std::cerr. Имя логу можно задать передав его в констуктор класса первым аргументом, а также опционально можно переназначить поток вывода передав нужный поток вторым аргументом.
Для удобства пользования определены макросы LOG_DURATION(имя-лога) и LOG_DURATION_STREAM(имя-лога, поток-вывода). Они создают объект класса LogDuration.

Для корректной работы многопоточных версий методов поискового сервера был реализован шаблонный класс ConcurrentMap со следующим функционалом:
  static_assert в начале класса не даст программе скомпилироваться при попытке использовать в качестве типа ключа что-либо, кроме целых чисел.
  Конструктор класса ConcurrentMap<Key, Value> принимает количество подсловарей, на которые надо разбить всё пространство ключей.
  operator[] ведет себя так же, как аналогичный оператор у map: если ключ key есть в словаре, возвращается объект класса Access, 
  содержащий ссылку на соответствующее ему значение. Если key в словаре нет, то добавляется пара (key, Value()) и возвращается объект класса Access, 
  содержащий ссылку на только что добавленное значение.
  Вспомогательная структура Access предоставляет ссылку на значение словаря и обеспечивает синхронизацию доступа к нему.
  Метод BuildOrdinaryMap сливает вместе все части словаря и возвращает весь словарь целиком в виде обычного контейнера map, при этом он
  потокобезопасный, то есть корректно работает, когда другие потоки выполняют операции с ConcurrentMap.