% CSV 파일을 읽어옵니다
%data = readtable('logging_data.csv');

% NaN 값을 가진 행을 제거합니다
%data = rmmissing(data);

% 첫 번째 그래프: Target_M_Pos와 Motor_Pos 비교
%figure;
%plot(data.Time, data.TargetMPos, 'r', 'DisplayName', 'Target M Pos');
%hold on;
%plot(data.Time, data.MotorPos, 'b', 'DisplayName', 'Motor Pos');
%xlabel('Time');
%ylabel('Position');
%title('Target M Pos vs Motor Pos');
%legend('Location', 'best');
%grid on;

% 두 번째 그래프: Target_Sim_Pos와 Motor_Sim_Pos 비교
%figure;
%plot(data.Time, data.TargetSimPos, 'r', 'DisplayName', 'Target Sim Pos');
%hold on;
%plot(data.Time, data.MotorSimPos, 'b', 'DisplayName', 'Motor Sim Pos');
%xlabel('Time');
%ylabel('Position');
%title('Target Sim Pos vs Motor Sim Pos');
%legend('Location', 'best');
%grid on;

% CSV 파일을 읽어옵니다
data = readtable('logging_data.csv');

% NaN 값을 가진 행을 제거합니다
% data = rmmissing(data);

% 첫 번째 그래프: Target_M_Pos와 Motor_Pos 비교
figure;
plot(data.Time, data.TargetMPos, 'r', 'DisplayName', 'Target M Pos');
hold on;
plot(data.Time, data.MotorPos, 'b', 'DisplayName', 'Motor Pos');
xlabel('Time');
ylabel('Position');
title('Target M Pos vs Motor Pos');
legend('Location', 'best');
grid on;

% RMS 에러 계산 (Target_M_Pos와 Motor_Pos 사이의 RMS 에러)
rms_error_M = sqrt(mean((data.TargetMPos - data.MotorPos).^2));
rms_desired_M = sqrt(mean((data.TargetMPos).^2));
rms_percent_M = rms_error_M/rms_desired_M *100;
disp(['RMS Error between Target M Pos and Motor Pos: ', num2str(rms_error_M)]);
disp(['RMS Error percentage: ', num2str(rms_percent_M)]);

% 두 번째 그래프: Target_Sim_Pos와 Motor_Sim_Pos 비교
figure;
plot(data.Time, data.TargetSimPos, 'r', 'DisplayName', 'Target Sim Pos');
hold on;
plot(data.Time, data.MotorSimPos, 'b', 'DisplayName', 'Motor Sim Pos');
xlabel('Time');
ylabel('Position');
title('Target Sim Pos vs Motor Sim Pos');
legend('Location', 'best');
grid on;

% RMS 에러 계산 (Target_Sim_Pos와 Motor_Sim_Pos 사이의 RMS 에러)
rms_error_Sim = sqrt(mean((data.TargetSimPos - data.MotorSimPos).^2));
rms_desired_Sim = sqrt(mean((data.TargetSimPos).^2));
rms_percent_Sim = rms_error_Sim/rms_desired_Sim *100;
disp(['RMS Error between Target Sim Pos and Motor Sim Pos: ', num2str(rms_error_Sim)]);
disp(['RMS Error percentage: ', num2str(rms_percent_Sim)]);

figure;
non_idx = find(data.Jitter <-900);
data.Jitter(non_idx)= [0];
boxplot(data.Jitter);
xlabel('Jitter Data');
title('Box Plot of Jitter(ms)');

figure;

plot(data.Time, data.Jitter, 'r', 'DisplayName', 'Target Sim Pos');
xlabel('time(s)');
title('Plot of Jitter(ms)');

mean(data.Jitter)
rmsJitter = sqrt(mean(data.Jitter.^2))
hold on;