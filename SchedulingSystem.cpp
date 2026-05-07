#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <map>
#include <set>
#include <algorithm>
#include <ctime>
#include <fstream>
#include <sstream>
#include <memory>
#include <iomanip>

using namespace std;

// Forward declarations
class Course;
class Student;
class Room;

// TimeSlot structure
struct TimeSlot {
    int day;    // 1-5 (Monday-Friday)
    int period; // 1-8 (class periods)

    TimeSlot(int d, int p) : day(d), period(p) {}

    bool operator<(const TimeSlot& other) const {
        return (day < other.day) || (day == other.day && period < other.period);
    }

    bool operator==(const TimeSlot& other) const {
        return (day == other.day && period == other.period);
    }
};

// Course class
class Course {
public:
    string courseCode;
    string name;
    string requiredRoomType;
    int maxCapacity;
    set<string> enrolledStudents;
    vector<TimeSlot> scheduledTimeSlots;
    string majorRequirement; // Which major this course is core for

    Course(string code, string n, string roomType, int capacity, string majorReq)
        : courseCode(code), name(n), requiredRoomType(roomType),
        maxCapacity(capacity), majorRequirement(majorReq) {
    }

    bool isFull() const {
        return enrolledStudents.size() >= maxCapacity;
    }

    bool hasTimeConflict(const vector<TimeSlot>& newSlots) const {
        for (const auto& newSlot : newSlots) {
            for (const auto& existingSlot : scheduledTimeSlots) {
                if (newSlot == existingSlot) {
                    return true;
                }
            }
        }
        return false;
    }

    bool enrollStudent(const string& studentId) {
        if (isFull()) return false;
        enrolledStudents.insert(studentId);
        return true;
    }

    void removeStudent(const string& studentId) {
        enrolledStudents.erase(studentId);
    }
};

// Student class
class Student {
public:
    string studentId;
    string major;
    int academicYear; // 1-3 (1stYear to 3rdYear)
    vector<string> enrolledCourses;
    time_t registrationTime;

    Student(string id, string m, int year)
        : studentId(id), major(m), academicYear(year) {
        registrationTime = time(nullptr);
    }

    bool isEnrolledIn(const string& courseCode) const {
        return find(enrolledCourses.begin(), enrolledCourses.end(), courseCode) != enrolledCourses.end();
    }

    string getYearName() const {
        switch (academicYear) {
        case 1: return "1stYear";
        case 2: return "2ndYear";
        case 3: return "3rdYear";
        default: return "Unknown";
        }
    }
};

// Room class
class Room {
public:
    string roomNumber;
    string type;
    int capacity;
    set<TimeSlot> availableTimeSlots;
    vector<string> specialEquipment;

    Room(string number, string t, int cap, const vector<TimeSlot>& availableSlots)
        : roomNumber(number), type(t), capacity(cap) {
        availableTimeSlots.insert(availableSlots.begin(), availableSlots.end());
    }

    bool isAvailable(const vector<TimeSlot>& requiredSlots) const {
        for (const auto& slot : requiredSlots) {
            if (availableTimeSlots.find(slot) == availableTimeSlots.end()) {
                return false;
            }
        }
        return true;
    }

    void reserveSlots(const vector<TimeSlot>& slots) {
        for (const auto& slot : slots) {
            availableTimeSlots.erase(slot);
        }
    }

    void releaseSlots(const vector<TimeSlot>& slots) {
        for (const auto& slot : slots) {
            availableTimeSlots.insert(slot);
        }
    }
};

// Registration Request structure
struct RegistrationRequest {
    string studentId;
    string courseCode;
    time_t timestamp;
    int priority;

    // Overload < operator for priority queue
    bool operator<(const RegistrationRequest& other) const {
        // Higher priority comes first
        if (priority != other.priority) {
            return priority < other.priority;
        }
        return timestamp > other.timestamp; // Earlier timestamp first if same priority
    }
};

// CourseScheduler class
class CourseScheduler {
private:
    map<string, shared_ptr<Course>> courses;
    map<string, shared_ptr<Student>> students;
    map<string, shared_ptr<Room>> rooms;
    priority_queue<RegistrationRequest> requestQueue;

    // Generate standard time slots for a course (2 periods back-to-back)
    vector<TimeSlot> generateTimeSlots(int day, int startPeriod) {
        return { TimeSlot(day, startPeriod), TimeSlot(day, startPeriod + 1) };
    }

    // Calculate priority for a request
    int calculatePriority(const Student& student, const Course& course) {
        int priority = 0;

        // Academic year (3rdYears get highest priority)
        priority += student.academicYear * 1000;

        // Core course for major
        if (student.major == course.majorRequirement) {
            priority += 500;
        }

        return priority;
    }

public:
    // Add objects to system
    void addCourse(shared_ptr<Course> course) {
        courses[course->courseCode] = course;
    }

    void addStudent(shared_ptr<Student> student) {
        students[student->studentId] = student;
    }

    void addRoom(shared_ptr<Room> room) {
        rooms[room->roomNumber] = room;
    }

    // Queue registration request
    void queueRegistrationRequest(const string& studentId, const string& courseCode) {
        auto studentIt = students.find(studentId);
        auto courseIt = courses.find(courseCode);

        if (studentIt == students.end() || courseIt == courses.end()) {
            cout << "Invalid student ID or course code!" << endl;
            return;
        }

        RegistrationRequest request;
        request.studentId = studentId;
        request.courseCode = courseCode;
        request.timestamp = time(nullptr);
        request.priority = calculatePriority(*studentIt->second, *courseIt->second);

        requestQueue.push(request);
        cout << "Registration request queued for student " << studentId
            << " for course " << courseCode << endl;
    }

    // Find suitable room for a course
    shared_ptr<Room> findSuitableRoom(Course& course, const vector<TimeSlot>& timeSlots) {
        for (auto& roomPair : rooms) {
            auto& room = roomPair.second;
            if (room->type == course.requiredRoomType &&
                room->capacity >= course.maxCapacity &&
                room->isAvailable(timeSlots)) {
                return room;
            }
        }
        return nullptr;
    }

    // Schedule a course
    bool scheduleCourse(const string& courseCode) {
        auto courseIt = courses.find(courseCode);
        if (courseIt == courses.end()) return false;

        auto& course = courseIt->second;

        // Try different time slots (Monday-Friday, periods 1-7)
        for (int day = 1; day <= 5; day++) {
            for (int period = 1; period <= 7; period++) {
                vector<TimeSlot> proposedSlots = generateTimeSlots(day, period);

                if (!course->hasTimeConflict(proposedSlots)) {
                    auto room = findSuitableRoom(*course, proposedSlots);
                    if (room) {
                        course->scheduledTimeSlots = proposedSlots;
                        room->reserveSlots(proposedSlots);
                        cout << "Scheduled course " << courseCode << " in room "
                            << room->roomNumber << " at day " << day
                            << ", period " << period << endl;
                        return true;
                    }
                }
            }
        }

        cout << "Could not schedule course " << courseCode << " - no suitable room/time available" << endl;
        return false;
    }

    // Process registration requests
    void processRequests() {
        cout << "\n=== PROCESSING REGISTRATION REQUESTS ===" << endl;
        int processed = 0;
        int successful = 0;

        while (!requestQueue.empty()) {
            RegistrationRequest request = requestQueue.top();
            requestQueue.pop();
            processed++;

            auto studentIt = students.find(request.studentId);
            auto courseIt = courses.find(request.courseCode);

            if (studentIt == students.end() || courseIt == courses.end()) {
                continue;
            }

            auto& student = studentIt->second;
            auto& course = courseIt->second;

            // Check if student is already enrolled
            if (student->isEnrolledIn(request.courseCode)) {
                continue;
            }

            // Schedule course if not already scheduled
            if (course->scheduledTimeSlots.empty()) {
                if (!scheduleCourse(request.courseCode)) {
                    continue; // Skip if couldn't schedule
                }
            }

            // Check for time conflicts with student's existing courses
            bool hasConflict = false;
            for (const auto& enrolledCourseCode : student->enrolledCourses) {
                auto enrolledCourse = courses[enrolledCourseCode];
                for (const auto& enrolledSlot : enrolledCourse->scheduledTimeSlots) {
                    for (const auto& newSlot : course->scheduledTimeSlots) {
                        if (enrolledSlot == newSlot) {
                            hasConflict = true;
                            break;
                        }
                    }
                    if (hasConflict) break;
                }
                if (hasConflict) break;
            }

            if (!hasConflict && !course->isFull()) {
                // Enroll student
                if (course->enrollStudent(request.studentId)) {
                    student->enrolledCourses.push_back(request.courseCode);
                    successful++;
                    cout << "Successfully enrolled student " << request.studentId
                        << " in course " << request.courseCode << endl;
                }
            }
            else {
                if (hasConflict) {
                    cout << "Time conflict for student " << request.studentId
                        << " in course " << request.courseCode << endl;
                }
                else if (course->isFull()) {
                    cout << "Course " << request.courseCode << " is full" << endl;
                }
            }
        }

        cout << "Processed " << processed << " requests, "
            << successful << " successful registrations" << endl;
    }

    // Balance class sizes (move students from overfull to underfull courses)
    void balanceClassSizes() {
        cout << "\n=== BALANCING CLASS SIZES ===" << endl;

        vector<shared_ptr<Course>> overfullCourses;
        vector<shared_ptr<Course>> underfullCourses;

        for (const auto& coursePair : courses) {
            auto& course = coursePair.second;
            int enrollment = course->enrolledStudents.size();
            int capacity = course->maxCapacity;

            if (enrollment > capacity * 0.9) { // Over 90% capacity
                overfullCourses.push_back(course);
            }
            else if (enrollment < capacity * 0.6) { // Under 60% capacity
                underfullCourses.push_back(course);
            }
        }

        for (auto& overfullCourse : overfullCourses) {
            for (auto& underfullCourse : underfullCourses) {
                if (overfullCourse->requiredRoomType == underfullCourse->requiredRoomType &&
                    !overfullCourse->scheduledTimeSlots.empty() &&
                    !underfullCourse->scheduledTimeSlots.empty()) {

                    // Check if time slots are compatible
                    bool timeCompatible = true;
                    for (const auto& slot1 : overfullCourse->scheduledTimeSlots) {
                        for (const auto& slot2 : underfullCourse->scheduledTimeSlots) {
                            if (slot1 == slot2) {
                                timeCompatible = false;
                                break;
                            }
                        }
                        if (!timeCompatible) break;
                    }

                    if (timeCompatible) {
                        // Try to move some students
                        vector<string> studentsToMove;
                        for (const auto& studentId : overfullCourse->enrolledStudents) {
                            // Check if student would have time conflict with underfull course
                            auto student = students[studentId];
                            bool hasConflict = false;
                            for (const auto& enrolledCourseCode : student->enrolledCourses) {
                                if (enrolledCourseCode != overfullCourse->courseCode) {
                                    auto enrolledCourse = courses[enrolledCourseCode];
                                    for (const auto& enrolledSlot : enrolledCourse->scheduledTimeSlots) {
                                        for (const auto& newSlot : underfullCourse->scheduledTimeSlots) {
                                            if (enrolledSlot == newSlot) {
                                                hasConflict = true;
                                                break;
                                            }
                                        }
                                        if (hasConflict) break;
                                    }
                                }
                                if (hasConflict) break;
                            }

                            if (!hasConflict) {
                                studentsToMove.push_back(studentId);
                                if (studentsToMove.size() >= 5) break; // Move max 5 at a time
                            }
                        }

                        // Actually move the students
                        for (const auto& studentId : studentsToMove) {
                            overfullCourse->removeStudent(studentId);
                            underfullCourse->enrollStudent(studentId);

                            // Update student's course list
                            auto student = students[studentId];
                            auto it = find(student->enrolledCourses.begin(),
                                student->enrolledCourses.end(),
                                overfullCourse->courseCode);
                            if (it != student->enrolledCourses.end()) {
                                *it = underfullCourse->courseCode;
                            }

                            cout << "Moved student " << studentId << " from "
                                << overfullCourse->courseCode << " to "
                                << underfullCourse->courseCode << endl;
                        }
                    }
                }
            }
        }
    }

    // Generate individual student schedule
    void generateStudentSchedule(const string& studentId) {
        auto studentIt = students.find(studentId);
        if (studentIt == students.end()) {
            cout << "Student not found!" << endl;
            return;
        }

        auto student = studentIt->second;
        cout << "\n=== SCHEDULE FOR STUDENT " << studentId << " ===" << endl;
        cout << "Major: " << student->major << ", Year: " << student->getYearName() << endl;
        cout << "\nEnrolled Courses:" << endl;

        for (const auto& courseCode : student->enrolledCourses) {
            auto course = courses[courseCode];
            cout << "- " << course->courseCode << ": " << course->name << endl;
            cout << "  Time: ";
            for (const auto& slot : course->scheduledTimeSlots) {
                cout << "Day " << slot.day << ", Period " << slot.period << " ";
            }
            cout << "\n  Room: ";
            // Find which room this course is in
            for (const auto& roomPair : rooms) {
                if (!roomPair.second->isAvailable(course->scheduledTimeSlots)) {
                    // This room has the time slots reserved, so course is here
                    cout << roomPair.first;
                    break;
                }
            }
            cout << endl;
        }
    }

    // Display all students in the system
    void displayAllStudents() {
        cout << "\n=== ALL STUDENTS ===" << endl;
        cout << "Total students: " << students.size() << endl;
        cout << "=============================================" << endl;
        cout << left << setw(10) << "ID" << setw(15) << "Major"
            << setw(12) << "Year" << setw(10) << "Courses" << endl;
        cout << "=============================================" << endl;

        for (const auto& studentPair : students) {
            auto student = studentPair.second;
            cout << left << setw(10) << student->studentId
                << setw(15) << student->major
                << setw(12) << student->getYearName()
                << setw(10) << student->enrolledCourses.size() << endl;
        }
        cout << "=============================================" << endl;
    }

    // Display detailed information about a specific student
    void displayStudentDetails(const string& studentId) {
        auto studentIt = students.find(studentId);
        if (studentIt == students.end()) {
            cout << "Student not found!" << endl;
            return;
        }

        auto student = studentIt->second;
        cout << "\n=== STUDENT DETAILS ===" << endl;
        cout << "ID: " << student->studentId << endl;
        cout << "Major: " << student->major << endl;
        cout << "Academic Year: " << student->getYearName() << endl;
        cout << "Number of enrolled courses: " << student->enrolledCourses.size() << endl;

        if (!student->enrolledCourses.empty()) {
            cout << "\nEnrolled Courses:" << endl;
            for (const auto& courseCode : student->enrolledCourses) {
                auto course = courses[courseCode];
                cout << "- " << course->courseCode << ": " << course->name;
                if (course->majorRequirement == student->major) {
                    cout << " (Core)";
                }
                cout << endl;
            }
        }
        else {
            cout << "\nNo courses enrolled yet." << endl;
        }
    }

    // Display students by major
    void displayStudentsByMajor(const string& major = "") {
        cout << "\n=== STUDENTS BY MAJOR ===" << endl;

        map<string, vector<shared_ptr<Student>>> studentsByMajor;

        for (const auto& studentPair : students) {
            studentsByMajor[studentPair.second->major].push_back(studentPair.second);
        }

        if (major.empty()) {
            // Show all majors
            for (const auto& majorGroup : studentsByMajor) {
                cout << "\nMajor: " << majorGroup.first << " ("
                    << majorGroup.second.size() << " students)" << endl;
                cout << "-------------------------" << endl;
                for (const auto& student : majorGroup.second) {
                    cout << student->studentId << " - " << student->getYearName() << endl;
                }
            }
        }
        else {
            // Show specific major
            auto it = studentsByMajor.find(major);
            if (it != studentsByMajor.end()) {
                cout << "\nMajor: " << major << " (" << it->second.size() << " students)" << endl;
                cout << "-------------------------" << endl;
                for (const auto& student : it->second) {
                    cout << student->studentId << " - " << student->getYearName()
                        << " - " << student->enrolledCourses.size() << " courses" << endl;
                }
            }
            else {
                cout << "No students found in major: " << major << endl;
            }
        }
    }

    // Display system status
    void displayStatus() {
        cout << "\n=== SYSTEM STATUS ===" << endl;
        cout << "Courses: " << courses.size() << endl;
        cout << "Students: " << students.size() << endl;
        cout << "Rooms: " << rooms.size() << endl;
        cout << "Pending requests: " << requestQueue.size() << endl;

        cout << "\nCourse Enrollment:" << endl;
        for (const auto& coursePair : courses) {
            auto course = coursePair.second;
            cout << course->courseCode << ": " << course->enrolledStudents.size()
                << "/" << course->maxCapacity << " students" << endl;
        }
    }
};

// Sample data initialization
void initializeSampleData(CourseScheduler& scheduler) {
    // Add rooms
    vector<TimeSlot> allSlots;
    for (int day = 1; day <= 5; day++) {
        for (int period = 1; period <= 8; period++) {
            allSlots.emplace_back(day, period);
        }
    }

    scheduler.addRoom(make_shared<Room>("R101", "Lecture", 50, allSlots));
    scheduler.addRoom(make_shared<Room>("R102", "Lecture", 40, allSlots));
    scheduler.addRoom(make_shared<Room>("LAB1", "Lab", 25, allSlots));
    scheduler.addRoom(make_shared<Room>("LAB2", "Lab", 30, allSlots));

    // Add courses
    scheduler.addCourse(make_shared<Course>("CS101", "Introduction to Programming", "Lecture", 45, "Computer Science"));
    scheduler.addCourse(make_shared<Course>("CS201", "Data Structures", "Lecture", 40, "Computer Science"));
    scheduler.addCourse(make_shared<Course>("CS301", "Algorithms", "Lecture", 35, "Computer Science"));
    scheduler.addCourse(make_shared<Course>("CS401", "AI", "Lecture", 30, "Computer Science"));
    scheduler.addCourse(make_shared<Course>("CSLAB1", "Programming Lab", "Lab", 25, "Computer Science"));

    scheduler.addCourse(make_shared<Course>("MATH101", "Calculus I", "Lecture", 40, "Mathematics"));
    scheduler.addCourse(make_shared<Course>("PHYS101", "Physics I", "Lecture", 35, "Physics"));

    // Add students
    scheduler.addStudent(make_shared<Student>("S002", "Computer Science", 3));
    scheduler.addStudent(make_shared<Student>("S003", "Computer Science", 2));
    scheduler.addStudent(make_shared<Student>("S004", "Computer Science", 1)); // 1stYears
    scheduler.addStudent(make_shared<Student>("S005", "Mathematics", 3));
    scheduler.addStudent(make_shared<Student>("S006", "Physics", 2));
    scheduler.addStudent(make_shared<Student>("S007", "Computer Science", 2));
    scheduler.addStudent(make_shared<Student>("S008", "Mathematics", 1));
    scheduler.addStudent(make_shared<Student>("S009", "Physics", 1));
    scheduler.addStudent(make_shared<Student>("S010", "Computer Science", 3));
}

// Main function
int main() {
    CourseScheduler scheduler;

    // Initialize with sample data
    initializeSampleData(scheduler);

    int choice;
    string studentId, courseCode, major;

    do {
        cout << "\n=== RICHFIELD COURSE SCHEDULER ===" << endl;
        cout << "1. Queue Registration Request" << endl;
        cout << "2. Process All Requests" << endl;
        cout << "3. Balance Class Sizes" << endl;
        cout << "4. Generate Student Schedule" << endl;
        cout << "5. Display All Students" << endl;
        cout << "6. Display Student Details" << endl;
        cout << "7. Display Students by Major" << endl;
        cout << "8. Display System Status" << endl;
        cout << "9. Schedule Specific Course" << endl;
        cout << "0. Exit" << endl;
        cout << "Enter choice: ";
        cin >> choice;

        switch (choice) {
        case 1:
            cout << "Enter student ID: ";
            cin >> studentId;
            cout << "Enter course code: ";
            cin >> courseCode;
            scheduler.queueRegistrationRequest(studentId, courseCode);
            break;
        case 2:
            scheduler.processRequests();
            break;
        case 3:
            scheduler.balanceClassSizes();
            break;
        case 4:
            cout << "Enter student ID: ";
            cin >> studentId;
            scheduler.generateStudentSchedule(studentId);
            break;
        case 5:
            scheduler.displayAllStudents();
            break;
        case 6:
            cout << "Enter student ID: ";
            cin >> studentId;
            scheduler.displayStudentDetails(studentId);
            break;
        case 7:
            cout << "Enter major (or press enter for all): ";
            cin.ignore();
            getline(cin, major);
            scheduler.displayStudentsByMajor(major);
            break;
        case 8:
            scheduler.displayStatus();
            break;
        case 9:
            cout << "Enter course code to schedule: ";
            cin >> courseCode;
            scheduler.scheduleCourse(courseCode);
            break;
        case 0:
            cout << "Exiting..." << endl;
            break;
        default:
            cout << "Invalid choice!" << endl;
        }
    } while (choice != 0);

    return 0;
}