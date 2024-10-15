#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <queue>
#include <string>
#include <algorithm>

using namespace std;

class Process {
public:
    string label;
    int burstTime;
    int arrivalTime;
    int queue;
    int priority;
    int originalBurstTime; // Almacena el Burst Time original
    int waitTime;
    int completionTime;
    int responseTime;
    int turnAroundTime;
    bool isCompleted;

    Process(string lbl, int bt, int at, int q, int pr)
        : label(lbl), burstTime(bt), arrivalTime(at), queue(q), priority(pr),
          originalBurstTime(bt), waitTime(0), completionTime(0), responseTime(-1),
          turnAroundTime(0), isCompleted(false) {}
};

// Funciones para ordenar procesos
bool compareByArrivalTime(const Process &a, const Process &b) {
    return a.arrivalTime < b.arrivalTime;
}

// Funtor para comparar por etiqueta
struct CompareByLabel {
    string label;
    CompareByLabel(const string &l) : label(l) {}
    bool operator()(const Process &proc) const {
        return proc.label == label;
    }
};

class Scheduler {
private:
    vector<Process> processes;
    queue<Process> queue1; // RR con quantum 3
    queue<Process> queue2; // RR con quantum 5
    vector<Process> queue3; // FCFS
    int currentTime; // Tiempo global

public:
    Scheduler() : currentTime(0) {}

    void addProcess(Process p) {
        processes.push_back(p);
    }

    void updateProcess(const Process& updatedProcess) {
        for (vector<Process>::iterator it = processes.begin(); it != processes.end(); ++it) {
            if (it->label == updatedProcess.label) {
                *it = updatedProcess;
                break;
            }
        }
    }

    void execute() {
        // Encolar procesos en sus respectivas colas
        for (vector<Process>::iterator it = processes.begin(); it != processes.end(); ++it) {
            Process &p = *it;
            switch (p.queue) {
                case 1: queue1.push(p); break;
                case 2: queue2.push(p); break;
                case 3: queue3.push_back(p); break;
            }
        }

        // Procesar cada cola en orden de prioridad
        processQueueRR(queue1, 3); // Cola 1: RR con quantum 3
        processQueueRR(queue2, 5); // Cola 2: RR con quantum 5
        processQueueFCFS(queue3);        // Cola 3: FCFS
    }

    void processQueueRR(queue<Process>& q, int quantum) {
        if (q.empty()) return;

        queue<Process> readyQueue = q;

        while (!readyQueue.empty()) {
            Process p = readyQueue.front();
            readyQueue.pop();

            // Si el proceso llega después del tiempo actual, avanzamos el tiempo
            if (p.arrivalTime > currentTime) {
                currentTime = p.arrivalTime;
            }

            // Si el proceso comienza por primera vez, establecemos el tiempo de respuesta
            if (p.responseTime == -1) {
                p.responseTime = currentTime;
            }

            // Ejecutar el proceso por 'quantum' o el tiempo restante, lo que sea menor
            int timeSlice = (p.burstTime < quantum) ? p.burstTime : quantum;
            p.burstTime -= timeSlice;
            currentTime += timeSlice;

            // Si el proceso ha terminado
            if (p.burstTime == 0) {
                p.completionTime = currentTime;
                p.turnAroundTime = p.completionTime - p.arrivalTime;
                p.waitTime = p.turnAroundTime - p.originalBurstTime;
                p.isCompleted = true;

                // Actualizar el proceso original en el vector
                updateProcess(p);

                cout << "Proceso " << p.label << " finalizó en el tiempo " << currentTime << endl;
            } else {
                // Si el proceso aún necesita tiempo de CPU, lo volvemos a encolar
                readyQueue.push(p);
            }
        }
    }

    void processQueueFCFS(vector<Process>& q) {
        if (q.empty()) return;

        // Ordenar los procesos por tiempo de llegada
        sort(q.begin(), q.end(), compareByArrivalTime);

        for (vector<Process>::iterator it = q.begin(); it != q.end(); ++it) {
            Process &p = *it;

            if (p.arrivalTime > currentTime) {
                currentTime = p.arrivalTime;
            }

            // Si el proceso comienza por primera vez, establecemos el tiempo de respuesta
            if (p.responseTime == -1) {
                p.responseTime = currentTime;
            }

            p.waitTime = currentTime - p.arrivalTime;
            currentTime += p.burstTime;
            p.completionTime = currentTime;
            p.turnAroundTime = p.completionTime - p.arrivalTime;
            p.isCompleted = true;

            // Actualizar el proceso original en el vector
            updateProcess(p);

            cout << "Proceso " << p.label << " completado en el tiempo " << currentTime << " (FCFS)" << endl;
        }
    }

    void printResults(const string& outputFilename) {
        ofstream outFile(outputFilename.c_str());
        outFile << "# etiqueta; BT; AT; Q; Pr; WT; CT; RT; TAT\n";

        double totalWT = 0, totalCT = 0, totalRT = 0, totalTAT = 0;

        for (vector<Process>::iterator it = processes.begin(); it != processes.end(); ++it) {
            Process &p = *it;
            outFile << p.label << ";" << p.originalBurstTime << ";" << p.arrivalTime << ";"
                    << p.queue << ";" << p.priority << ";"
                    << p.waitTime << ";" << p.completionTime << ";"
                    << p.responseTime << ";" << p.turnAroundTime << "\n";

            totalWT += p.waitTime;
            totalCT += p.completionTime;
            totalRT += p.responseTime;
            totalTAT += p.turnAroundTime;
        }

        size_t n = processes.size();

        outFile << "WT=" << totalWT / n << "; CT=" << totalCT / n << "; RT=" << totalRT / n << "; TAT=" << totalTAT / n << ";\n";
        outFile.close();
    }
};

void loadProcessesFromFile(Scheduler& scheduler, const string& filename) {
    ifstream file(filename.c_str());

    if (!file.is_open()) {
        cerr << "Error al abrir el archivo: " << filename << endl;
        exit(1);
    }

    string line;

    while (getline(file, line)) {
        if (line.empty() || line[0] == '#') continue; // Saltar líneas vacías y comentarios

        stringstream ss(line);
        string label;
        int bt, at, q, pr;
        char delimiter;

        // Leer etiqueta
        getline(ss, label, ';');
        // Leer Burst Time
        ss >> bt;
        ss >> delimiter;
        // Leer Arrival Time
        ss >> at;
        ss >> delimiter;
        // Leer número de cola
        ss >> q;
        ss >> delimiter;
        // Leer prioridad
        ss >> pr;

        Process p(label, bt, at, q, pr);
        scheduler.addProcess(p);
    }
}

string generateOutputFilename(const string& inputFilename) {
    // Encontrar la posición del último separador de ruta
    size_t lastSlashPos = inputFilename.find_last_of("/\\");
    string filenameOnly = inputFilename.substr(lastSlashPos + 1);

    // Generar el nombre del archivo de salida
    string outputFilename = "output_" + filenameOnly;
    return outputFilename;
}

int main(int argc, char* argv[]) {
     if (argc < 2) {
        cout << "Uso: " << argv[0] << " <archivo_de_entrada>\n";
        return 1;
    }
    string filename = argv[1];

    Scheduler scheduler;
    loadProcessesFromFile(scheduler, filename);
    scheduler.execute();

    string outputFilename = generateOutputFilename(filename);
    scheduler.printResults(outputFilename);

    return 0;
}
